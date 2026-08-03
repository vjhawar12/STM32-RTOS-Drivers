# STM32 Vehicle Telemetry Node

A FreeRTOS-based telemetry and diagnostics node for an off-road vehicle, built on the STM32F401RE. The firmware combines chassis acceleration with suspension-travel measurements, classifies vehicle events, emits timestamped UART telemetry, and supervises runtime health with task check-ins, stack/heap diagnostics, assertions, and an independent watchdog.

The repository is organized around the integrated vehicle-telemetry application. The original FreeRTOS exercises remain under `labs/` as reference material.

## Sensor Use Case

The node combines two complementary measurements:

- **ADXL345 accelerometer:** measures chassis acceleration, vibration, rough-terrain severity, and short-duration shock or impact events.
- **VL6180 time-of-flight sensor:** measures suspension displacement by tracking the distance between the chassis and a suspension member, such as the axle or control arm.

The ToF sensor is intended to be mounted parallel to the shock absorber and away from direct mud exposure. Changes in measured distance represent compression and rebound. Correlating that travel with acceleration provides more context than either sensor alone: a large acceleration with bottomed-out suspension can indicate a hard landing, while a severe acceleration without extreme suspension travel can indicate a direct obstacle impact.

## Current Runtime Architecture

```text
TIM2 periodic interrupt (100 ms)                 VL6180 continuous ranging
              │                                             │
              v                                             v
  notify accelerometer task                    GPIO1 active-low interrupt
              │                                             │
              v                                             v
      ADXL345 SPI sample                         PA4 / EXTI4 ISR
              │                                             │
              │                                  notify ToF task
              │                                             │
              │                                  I2C read + IRQ clear
              └──────────────────┬──────────────────────────┘
                                 v
                         sensor_queue
                                 │
                                 v
                        processing task
                 sensor state + combined state
                                 │
                                 v
                         message_queue
                                 │
                                 v
                         UART logger task
                                 │
                                 v
                              USART2

500 ms software timer → health task ─┐
sensor/processing task check-ins ────┼─> event group → watchdog task → IWDG
                                     ┘
```

Only the logger task writes normal runtime messages to UART. Producer tasks copy null-terminated messages into a fixed-capacity FreeRTOS queue, preventing local buffers from being overwritten before transmission.

## Implemented Features

- STM32F401RE firmware written in C using CMSIS and FreeRTOS
- Register-level drivers for SPI, I2C, UART, timers, GPIO/EXTI, and the independent watchdog
- Custom ADXL345 driver with timer-triggered acquisition
- Custom VL6180 driver with continuous ranging and GPIO1/EXTI4 sample-ready notifications
- Independent sensor-acquisition tasks using direct-to-task notifications
- Typed queue carrying acceleration and ToF samples to the processing task
- Timestamped human-readable UART telemetry
- Dedicated queue-based UART logger task
- Dropped-log tracking when the message queue cannot accept a valid message
- Suspension-state classification:
  - near bottom-out
  - normal travel
  - near full extension
  - unknown/invalid
- Acceleration-state classification:
  - normal vibration
  - moderate impact
  - severe impact
  - crash-level impact
  - unknown/invalid
- Combined vehicle states including normal driving, airborne, hard landing, severe landing, obstacle impact, crash, and fault
- Runtime health telemetry for stack high-water marks, free heap, uptime, and sensor-queue capacity
- Event-group task check-ins used to decide whether the watchdog may be refreshed
- `configASSERT()` failure capture with source file, source line, Cortex-M fault/status registers, stack pointers, and a debugger breakpoint
- 16 KiB FreeRTOS heap for dynamically created task stacks, queues, timers, event groups, and task-control blocks

## Current Classification Assumptions

The current firmware uses initial development thresholds rather than calibrated vehicle-specific values:

- Suspension reference range: **30–200 mm**
- Near bottom-out: lowest **8%** of that configured range
- Near full extension: highest **8%** of that configured range
- Acceleration delta bands: normal, moderate, severe, and crash thresholds
- Combined states are generated when the latest valid acceleration and ToF timestamps are within an **800 ms** window

These values are placeholders for bench and vehicle calibration. The next signal-processing stage should replace the broad latest-sample window with timestamp-aligned event windows.

## Repository Structure

```text
apps/
  vehicle_telemetry/     Integrated telemetry application

common/
  drivers/               Register-level peripheral and sensor drivers
  inc/                   Shared interfaces and FreeRTOS configuration
  src/                   STM32 platform, interrupt, and exception support

labs/                    FreeRTOS learning examples
freertos/                FreeRTOS kernel and portable layer
cmsis/                   CMSIS and STM32 device support

docs/
  architecture.md        Current data flow and planned transmission architecture
```

## Build and Flash

Build the telemetry application:

```bash
./build.sh
```

Flash the current application:

```bash
./flash.sh
```

Build an individual FreeRTOS lab:

```bash
./build.sh -DBUILD_LAB=ON -DLAB_NAME=<lab_name>
```

## Next Development Stage

The current baseline ends at human-readable UART telemetry. The next stage is structured data transmission and event analysis:

1. Add timestamp-aligned sample/event windows.
2. Calculate suspension velocity and compression/rebound direction.
3. Record event duration, peak acceleration, and minimum/maximum travel.
4. Define a versioned binary telemetry packet with sequence numbers, validity flags, counters, and CRC.
5. Add a dedicated non-blocking communication queue and task.
6. Implement the UART protocol to a Wi-Fi coprocessor.
7. Transmit telemetry over UDP.
8. Add a host receiver, decoder, live plots, and recorded-event visualization.
9. Add fault injection and long-duration reliability testing.

See [`docs/architecture.md`](docs/architecture.md) for the detailed current and target architecture.
