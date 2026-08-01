# STM32 Vehicle Telemetry and Health-Monitoring Node

A FreeRTOS-based embedded telemetry node built on the STM32F401RE. The system acquires motion and proximity data using custom register-level drivers, classifies operating conditions, reports timestamped telemetry, and supervises task health using event groups and the independent watchdog.

The repository is organized around the integrated deliverable. The earlier FreeRTOS exercises remain available under `labs/` as supporting examples and development history.

## System Overview

```text
ADXL345 over SPI ──> accelerometer task ──┐
                                          ├──> sensor queue
VL6180 over I2C ──> proximity task ───────┘         │
                                                     v
                                           state processing task
                                                     │
                                                     v
                                                UART telemetry

critical task check-ins ──> event group ──> watchdog supervisor
                                     │
                                     └──> stack / heap / queue reporting
```

## Current Capabilities

- STM32F401RE firmware written in C with CMSIS and FreeRTOS
- Register-level SPI2, I2C, UART, timer, GPIO-interrupt, and IWDG support
- ADXL345 accelerometer driver with multi-byte XYZ acquisition
- VL6180 time-of-flight distance sensor driver
- Independent sensor acquisition tasks
- ISR-to-task signaling using task notifications
- Queue-based sensor data transport
- Rule-based motion, proximity, and combined-system state classification
- Timestamped UART telemetry
- Stack high-water-mark, heap, uptime, and queue-space reporting
- Conditional watchdog refresh after required tasks report liveness

## Repository Layout

```text
apps/
  vehicle_telemetry/
    src/main.c              Product-level firmware entry point

common/
  drivers/                  Reusable bus and sensor drivers
  inc/                      Platform and interrupt interfaces
  src/                      STM32F401RE platform implementation

labs/                       Standalone FreeRTOS and driver exercises
freertos/                   FreeRTOS kernel sources and port
cmsis/                      CMSIS core and STM32 device support
linker/                     STM32F401RE linker script
photos/                     Hardware and logic-analyzer evidence
```

## Build the Telemetry Application

The telemetry application is the default target.

```bash
./build.sh
```

This produces:

```text
build/vehicle_telemetry.elf
build/vehicle_telemetry.hex
build/vehicle_telemetry.bin
build/STM32_Vehicle_Telemetry.map
```

Flash it with:

```bash
./flash.sh
```

## Build an Existing Lab

The labs are preserved and can be selected explicitly:

```bash
./build.sh -DBUILD_LAB=ON -DLAB_NAME=queues
```

Flash the selected lab by passing its target name:

```bash
./flash.sh queues
```

## Low-Level Driver Work

- `common/drivers/spi.c` — SPI2 clock, GPIO alternate functions, mode configuration, chip-select control, and blocking transfers
- `common/drivers/adxl345.c` — register access, device identity check, configuration, and XYZ sample acquisition
- `common/drivers/i2c.c` — register-level I2C transactions including 16-bit register addressing
- `common/drivers/vl6180.c` — device identity, ranging configuration, continuous measurements, and interrupt-driven task notification
- `common/drivers/uart.c` — USART2 initialization and serial input/output

## Hardware Validation

The firmware has been exercised on an STM32 Nucleo board with external sensor modules. I2C activity was inspected using a logic analyzer and protocol decoder during sensor bring-up.

<p align="center">
  <img src="/photos/sensor_breadboard.jpg" width="750">
</p>

<p align="center">
  <img src="/photos/i2c_logic_analyzer.jpg" width="750">
</p>

<p align="center">
  <img src="/photos/i2c_protocol.jpg" width="750">
</p>

<p align="center">
  <img src="/photos/stm32_setup.jpeg" width="750">
</p>

## Development Roadmap

The next major milestone is wireless telemetry:

1. Define a versioned telemetry packet with sequence number, timestamp, health flags, and checksum.
2. Add a dedicated network transmit queue and communications task.
3. Interface the STM32 with a Wi-Fi coprocessor over a separate UART.
4. Transmit telemetry over UDP without blocking sensor acquisition.
5. Add a Python receiver for packet validation, logging, and live visualization.
6. Add reconnection handling, packet-loss counters, stale-data detection, and fault-injection tests.

See `docs/architecture.md` for the intended module boundaries.

## Supporting FreeRTOS Examples

The `labs/` directory documents the progression used to build the final system, including:

- Task creation, priorities, and periodic scheduling
- Queues, semaphores, mutexes, notifications, and timers
- Interrupt-to-task signaling
- UART command handling
- SPI accelerometer integration
- I2C distance-sensor integration
- Multi-sensor processing
- Runtime health monitoring and watchdog supervision

These remain buildable examples, but the primary deliverable is now `apps/vehicle_telemetry`.
