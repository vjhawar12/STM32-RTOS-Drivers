# Vehicle Telemetry Architecture

## Current Firmware Boundary

The STM32 currently owns all real-time and safety-relevant behavior:

- timer- and interrupt-driven sensor acquisition
- sample timestamps and validity
- suspension and acceleration classification
- combined vehicle-state evaluation
- message formatting and queued UART output
- runtime health diagnostics
- task-liveness supervision
- watchdog policy
- assertion evidence for debugger inspection

The current transport is human-readable USART2 output. Structured packets, a Wi-Fi coprocessor protocol, UDP transport, and host visualization are the next milestone.

## Current RTOS Objects

| Object | Current role |
|---|---|
| ToF task, priority 7 | Waits for a VL6180 GPIO1 notification, reads the range over I2C, clears the sensor interrupt, validates the sample, and publishes it |
| Accelerometer task, priority 7 | Waits for the TIM2 notification, reads the ADXL345 over SPI, classifies the acceleration delta, and publishes it |
| Processing task, priority 5 | Receives mixed sensor samples, formats sensor telemetry, and evaluates the combined vehicle state |
| Health task, priority 3 | Runs from a 500 ms software-timer notification and reports stack, heap, uptime, and queue diagnostics |
| UART logger task, priority 2 | Owns normal UART transmission and drains the message queue |
| Watchdog task, priority 8 | Requires all critical task check-ins before refreshing the independent watchdog |
| `sensor_queue`, length 8 | Carries copied `mixed_sensor_data_t` samples from both sensor tasks to processing |
| `message_queue`, length 10 | Carries copied, null-terminated fixed-capacity log messages to the UART task |
| Event group | Records ToF, acceleration, processing, and health check-ins for each watchdog interval |
| Health software timer | Notifies the health task every 500 ms |

All application tasks and RTOS objects are created dynamically. Their stacks and control structures therefore consume the FreeRTOS heap configured in `common/inc/FreeRTOSConfig.h`.

## Current Runtime Data Flow

```text
TIM2 update interrupt
        │
        v
periodic_timer_isr()
        │ direct task notification
        v
accelerometer task
        │ SPI read + timestamp + validity/state
        │
        ├──────────────────────────────┐
        │                              │
        │                        sensor_queue
        │                              │
        │                              v
        │                       processing task
        │                    latest valid sensor states
        │                              │
        │                     combined state evaluation
        │                              │
        │                              v
        │                        message_queue
        │                              │
        │                              v
        │                        UART logger task
        │                              │
        │                              v
        │                           USART2
        │
VL6180 continuous ranging
        │
        v
GPIO1 active-low assertion
        │
        v
PA4 / EXTI4_IRQHandler()
        │ clear STM32 EXTI pending flag
        │ direct task notification
        v
ToF task
        │ I2C range/status read
        │ clear VL6180 interrupt flag
        │ timestamp + validity/state
        └──────────────────────────────┘
```

## Interrupt Ownership and Acknowledgement

The ToF path contains two distinct pending states:

1. **STM32 EXTI4 pending flag**
   - Set when PA4 observes the configured falling edge.
   - Cleared in `EXTI4_IRQHandler()` by writing `1` to bit 4 of `EXTI->PR`.

2. **VL6180 range interrupt flag**
   - Generated when `SYSTEM__INTERRUPT_CONFIG_GPIO` selects “new range sample ready.”
   - Exposed on GPIO1 by `SYSTEM__MODE_GPIO1` as an active-low interrupt output.
   - Cleared from the ToF task by writing the range-clear bit to `SYSTEM__INTERRUPT_CLEAR` over I2C.

The ISR performs no I2C transaction. It only acknowledges the STM32 interrupt and wakes the ToF task.

The accelerometer path uses TIM2 as a 100 ms trigger. The ISR notifies the accelerometer task, keeping the SPI transaction in task context rather than interrupt context.

## Message Ownership and UART Logging

Normal runtime producers do not call `uart_outstring()` directly. They create a local formatted string and call `store_message()`.

Each queued item contains:

```c
typedef struct message_t {
    uint16_t size;               /* Includes the null character. */
    char buffer[MAX_MESSAGE_SIZE];
} message_t;
```

FreeRTOS copies the complete structure into `message_queue`. This gives each pending message independent storage even after the producer reuses its local buffer.

`store_message()` returns one of three states:

- `INVALID_MESSAGE`: empty or larger than the accepted fixed capacity
- `MESSAGE_STORED`: copied into the queue
- `MESSAGE_DROPPED`: valid message, but the queue remained full for the configured send timeout

Dropped messages increment a diagnostic counter rather than stopping sensor acquisition.

## Current State Model

### Suspension state

The current development configuration assumes:

```text
compressed reference: 30 mm
extended reference:   200 mm
near bottom-out:       lowest 8% of configured travel
normal travel:         middle 84%
near full extension:   highest 8%
```

A failed notification or invalid VL6180 range status produces `SUS_UNKNOWN`.

### Acceleration state

The ADXL345 driver supplies an acceleration delta used for the current bands:

```text
0–3       normal vibration
>3–6      moderate impact
>6–12     severe impact
>12       crash-level impact
invalid   unknown
```

### Combined state

The processing task currently holds the latest valid ToF and acceleration state. When their timestamps differ by less than 800 ms, it maps the pair into:

- `SYS_NORMAL`
- `SYS_AIRBORNE`
- `SYS_HARD_LANDING`
- `SYS_SEVERE_LANDING`
- `SYS_OBSTACLE_IMPACT`
- `SYS_CRASH`
- `SYS_FAULT`

The 800 ms latest-sample window is a baseline implementation. It should be replaced by short event windows that retain multiple samples and measure event timing, peaks, and duration.

## Health, Watchdog, and Assertions

### Health task

The 500 ms health callback notifies a dedicated task that currently records:

- ToF task stack high-water mark
- accelerometer task stack high-water mark
- processing task stack high-water mark
- current free FreeRTOS heap
- uptime
- free spaces in `sensor_queue`

Future health output should also report minimum-ever free heap, logger/watchdog stack high-water marks, sensor timeout counters, and dropped-message count.

### Watchdog policy

The watchdog task waits up to one second for the following event bits:

```text
TOF_BIT | ACCEL_BIT | PROCESSING_BIT | HEALTH_BIT
```

Only a complete set permits an IWDG reload. Missing check-ins are logged, and the watchdog remains responsible for recovery if critical execution does not resume.

### Assertion evidence

`configASSERT()` calls `vHandleException(file, line)`. The exception record stores source location plus Cortex-M debug state such as:

- CFSR and HFSR
- BFAR and MMFAR
- ICSR, SHCSR, and VTOR
- xPSR, PSP, and MSP
- captured LR and PC values

The handler executes `BKPT` and then preserves the record in a halt loop for inspection through a debug probe.

## Target Module Layout

The current integrated `main.c` is useful while behavior is still changing, but the product application should be split as the data-transmission stage grows:

```text
apps/vehicle_telemetry/
  inc/
    telemetry_types.h
    vehicle_state.h
    sensor_tasks.h
    event_window.h
    health_monitor.h
    telemetry_output.h
    watchdog_supervisor.h
    network_transport.h
  src/
    main.c
    vehicle_state.c
    sensor_tasks.c
    event_window.c
    health_monitor.c
    telemetry_output.c
    watchdog_supervisor.c
    network_transport.c
```

`main.c` should eventually contain only platform initialization, RTOS object creation, task creation, driver startup, and scheduler startup.

## Target Transmission Data Flow

```text
sensor tasks
     │
     v
sample/event queue
     │
     v
timestamp-aligned event window
     │
     ├─ suspension velocity and direction
     ├─ peak acceleration
     ├─ minimum/maximum travel
     ├─ event duration
     └─ classified event
     │
     v
versioned telemetry_packet_t
     │
     v
non-blocking transmit queue
     │
     v
communication task
     │ UART protocol
     v
Wi-Fi coprocessor
     │ UDP
     v
host receiver / live dashboard / recording
```

Sensor acquisition and watchdog behavior must not depend on network availability. A full transmit queue should increment a counter and drop or coalesce telemetry according to policy rather than block critical tasks.

## Planned Telemetry Packet

The packet should include:

- magic value
- protocol version
- message type
- node identifier
- sequence number
- timestamp or uptime
- acceleration axes and derived magnitude/delta
- suspension distance and derived velocity/direction
- sensor and combined states
- validity and health flags
- event duration and peak values
- dropped-sample, dropped-message, and network-error counters
- checksum or CRC

The wire format should be declared once in `telemetry_types.h` and mirrored by the host decoder. Explicit integer widths and byte order should be used; raw compiler-dependent C structures should not be transmitted without serialization rules.

## Recommended Implementation Order

1. Add counters for ToF notification timeouts, sensor failures, queue failures, and minimum-ever free heap.
2. Introduce shared telemetry, fault, and packet types.
3. Extract the combined state estimator into a pure C module with table-driven tests.
4. Add a fixed-size timestamped event ring buffer.
5. Derive suspension velocity, direction, peaks, and event duration.
6. Define and test the versioned packet serializer and CRC.
7. Add a non-blocking communication queue and task.
8. Implement the UART state machine for the Wi-Fi coprocessor.
9. Add UDP transport, host decoding, live plotting, and recording.
10. Add fault-injection and long-duration reliability tests.
