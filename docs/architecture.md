# Vehicle Telemetry Architecture

## Current Firmware Boundary

The STM32 owns all real-time and safety-relevant behavior:

- sensor acquisition
- timestamps and sample validity
- state classification
- runtime diagnostics
- task-liveness supervision
- watchdog policy
- telemetry packet creation

The current application sends human-readable telemetry over UART. The next milestone replaces direct logging from producer tasks with a dedicated telemetry pipeline.

## Target Module Layout

```text
apps/vehicle_telemetry/
  inc/
    telemetry_types.h
    vehicle_state.h
    sensor_tasks.h
    health_monitor.h
    telemetry_output.h
    watchdog_supervisor.h
    network_transport.h
  src/
    main.c
    vehicle_state.c
    sensor_tasks.c
    health_monitor.c
    telemetry_output.c
    watchdog_supervisor.c
    network_transport.c
```

`main.c` should eventually contain only platform initialization, RTOS object creation, task creation, and scheduler startup.

## Runtime Data Flow

```text
accelerometer ISR/timer                proximity interrupt
          │                                    │
          v                                    v
accelerometer task                    proximity task
          │                                    │
          └──────── telemetry_sample_t ─────────┘
                           │
                           v
                    processing queue
                           │
                           v
                 vehicle-state estimator
                           │
                   telemetry_packet_t
                           │
                           v
                  network transmit queue
                           │
                           v
                     network task
                           │
                       UART link
                           │
                           v
                    Wi-Fi coprocessor
                           │
                          UDP
                           │
                           v
                   host receiver/dashboard
```

Sensor and processing tasks must never block on Wi-Fi state. If the network queue fills, the firmware should increment a dropped-packet counter and continue acquiring data.

## Planned Telemetry Packet

The packet should include:

- magic value
- protocol version
- message type
- node identifier
- sequence number
- uptime or timestamp
- acceleration axes
- proximity distance
- current motion, proximity, and combined state
- sensor-validity and health flags
- dropped-sample and network-error counters
- checksum or CRC

The exact wire format should be declared once in `telemetry_types.h` and mirrored by the host decoder.

## Health Model

Task liveness, sensor validity, and network availability are separate concerns:

- A task can run while a sensor returns invalid data.
- A sensor can be healthy while the network is disconnected.
- A network outage should not stop acquisition or trigger an immediate watchdog reset.
- A stalled critical task should prevent watchdog refresh.

Recommended diagnostic counters include:

- sensor read failures
- trigger timeouts
- stale samples
- telemetry queue overflows
- network queue overflows
- Wi-Fi reconnect attempts
- UDP transmit failures
- watchdog reset cause

## Implementation Order

1. Extract the state estimator into a pure C module.
2. Introduce shared telemetry and fault types.
3. Move UART formatting into one output task.
4. Add sequence numbers, health flags, and counters.
5. Add a non-blocking network transmit queue.
6. Implement the Wi-Fi coprocessor UART state machine.
7. Add the UDP receiver and visualization tool.
8. Add host-side tests and fault-injection demonstrations.
