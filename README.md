# STM32 Vehicle Telemetry Node

A FreeRTOS-based embedded telemetry system built on the STM32F401RE for an off-road vehicle. The firmware collects chassis acceleration and suspension-travel measurements, processes vehicle condition data, reports telemetry, and monitors system health using RTOS primitives and watchdog supervision.

The repository is organized around the integrated telemetry application. Supporting FreeRTOS exercises remain available under `labs/` as reference material.

## Sensor Use Case

The node combines two complementary measurements:

- **ADXL345 accelerometer:** measures chassis acceleration, vibration, rough-terrain severity, and short-duration shock or impact events.
- **VL6180 time-of-flight sensor:** measures suspension displacement by tracking the distance between the chassis and a suspension member, such as the axle or control arm.

Changes in the ToF measurement represent suspension compression and rebound. Rapid compression, unusually low clearance, or repeated travel near the mechanical limit can be used to identify harsh suspension events and possible bottom-out conditions. Correlating suspension travel with acceleration provides a more useful view of vehicle shock loading than either sensor alone.

## System Architecture

```text
ADXL345 (SPI) ──> chassis acceleration task ─────┐
                                                 │
                                                 v
                                          processing queue
                                                 │
VL6180 (I2C) ──> suspension-travel task ─────────┘
                                                 │
                                                 v
                                      vehicle condition processing
                                                 │
                                                 v
                                          telemetry output

Task health monitoring
        │
        v
Event groups → Watchdog supervisor
```

## Features

- STM32F401RE firmware written in C using CMSIS and FreeRTOS
- Register-level peripheral drivers for SPI, I2C, UART, timers, GPIO interrupts, and watchdog
- Custom ADXL345 accelerometer driver for chassis vibration and shock measurements
- Custom VL6180 ToF driver for suspension-travel measurements
- Independent RTOS sensor-acquisition tasks
- Queue-based data transfer between tasks
- Timestamped acceleration and suspension telemetry
- Vehicle-condition classification based on vibration, impact, suspension compression, and sensor validity
- UART telemetry output
- Runtime health monitoring:
  - stack usage
  - heap usage
  - queue status
  - task liveness
- Watchdog supervision using task check-ins

## Repository Structure

```text
apps/
  vehicle_telemetry/     Main firmware application

common/
  drivers/               Reusable peripheral and sensor drivers
  inc/                   Shared interfaces
  src/                   Platform code

labs/                    FreeRTOS learning examples
freertos/                FreeRTOS kernel
cmsis/                   CMSIS and STM32 support
```

## Build and Flash

Build the telemetry application:

```bash
./build.sh
```

Flash:

```bash
./flash.sh
```

Build an individual FreeRTOS lab:

```bash
./build.sh -DBUILD_LAB=ON -DLAB_NAME=<lab_name>
```

## Future Work

The next development stage is extending the telemetry node into a networked embedded system:

- calibrated suspension-travel and shock thresholds
- suspension compression, rebound, and bottom-out event detection
- versioned telemetry packet format
- dedicated communication task
- Wi-Fi coprocessor interface
- UDP telemetry transport
- host-side receiver and live visualization tools
- fault injection and reliability testing

See `docs/architecture.md` for the planned system evolution.
