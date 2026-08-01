# STM32 Vehicle Telemetry Node

A FreeRTOS-based embedded telemetry system built on the STM32F401RE. The firmware collects motion and proximity data, processes sensor state, reports telemetry, and monitors system health using RTOS primitives and watchdog supervision.

The repository is organized around the integrated telemetry application. Supporting FreeRTOS exercises remain available under `labs/` as reference material.

## System Architecture

```text
ADXL345 (SPI) ───────┐
                     │
                     v
              Sensor acquisition tasks
                     │
VL6180 (I2C) ─────────┘
                     │
                     v
              Processing task
                     │
                     v
              Telemetry output

Task health monitoring
        │
        v
Event groups → Watchdog supervisor
```

## Features

- STM32F401RE firmware written in C using CMSIS and FreeRTOS
- Register-level peripheral drivers for SPI, I2C, UART, timers, GPIO interrupts, and watchdog
- Custom ADXL345 accelerometer driver
- Custom VL6180 time-of-flight distance sensor driver
- RTOS task-based sensor acquisition pipeline
- Queue-based data transfer between tasks
- Motion and proximity state classification
- UART telemetry output with timestamps
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

- versioned telemetry packet format
- dedicated communication task
- Wi-Fi coprocessor interface
- UDP telemetry transport
- host-side receiver and visualization tools
- fault injection and reliability testing

See `docs/architecture.md` for the planned system evolution.
