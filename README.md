# STM32 RTOS Drivers with FreeRTOS

A hands-on embedded firmware lab series focused on **FreeRTOS**, **register-level STM32 drivers**, and **hardware/software integration** on microcontrollers.

This repository builds from core RTOS primitives into practical embedded applications: task scheduling, deterministic timing, queues, semaphores, mutexes, interrupt-to-task signaling, peripheral driver integration, and system diagnostics.

## Highlights

- Register-level STM32F401RE peripheral configuration using CMSIS
- FreeRTOS task scheduling, queues, semaphores, mutexes, timers, and task notifications
- Custom SPI driver and ADXL345 accelerometer driver
- Custom I2C driver and VL6180 TOF sensor driver
- Custom UART driver
- RTOS-based sensor acquisition and processing pipeline
- Modular driver/application separation for reusable embedded firmware

---

## Low-Level Driver Work

This repo includes multiple drivers: 

1. A register-level SPI2 driver for the STM32F401RE
2. An ADXL345 accelerometer driver layered on the SPI driver
3. A register-level I2C driver for the STM32F401RE
4. A VL6180 TOF sensor driver layered on the I2C driver
5. A UART driver used throughout the project

Written in the following files: 

- `common/drivers/spi.c` — SPI2 initialization, GPIO alternate-function setup, chip-select control, and blocking SPI transfers
- `common/drivers/adxl345.c` — ADXL345 register access, device ID check, measurement configuration, and multi-byte XYZ sample reads
- `common/drivers/i2c.c` — I2C initialization, GPIO alternate-function setup, ACK/NACK reception, and blocking I2C transfers
- `common/drivers/vl6180.c` — VL6180 register access, device ID check, measurement configuration, and multi-byte XYZ sample reads
- `common/drivers/uart.c` — UART initialization, char/byte/string transfer, blocking and non-blocking functions

This demonstrates direct peripheral configuration, reusable bus-driver design, and device-driver integration under FreeRTOS.

---

## Hardware Photos

<p align="center">
  <img src="/photos/sensor_breadboard.jpg" width="750">
</p>

<p align="center"><em>
Breadboard hardware setup for the STM32 sensor-interface labs, with an external sensor module wired for I2C validation.
</em></p>

<p align="center">
  <img src="/photos/i2c_logic_analyzer.jpg" width="750">
</p>

<p align="center"><em>
Logic-analyzer capture of an I2C transaction used to validate signal timing and bus activity during sensor bring-up.
</em></p>

<p align="center">
  <img src="/photos/i2c_protocol.jpg" width="750">
</p>

<p align="center"><em>
WaveForms protocol-decoder output showing decoded I2C traffic, including address, write transaction, ACK behavior, and transmitted data bytes.
</em></p>

<p align="center">
  <img src="/photos/stm32_setup.jpg" width="750">
</p>

<p align="center"><em>
STM32 Nucleo Board setup with laptop for UART communication.
</em></p>

## Lab Progression

### RTOS Foundations

| Lab | Topic | Concepts |
|---|---|---|
| Lab 1 | Multiple Tasks and Priorities | task creation, scheduling, priorities, stack sizing |
| Lab 2 | Periodic Tasks | `vTaskDelayUntil()`, deterministic timing, periodic execution |
| Lab 3 | Queues | producer/consumer design, inter-task data transfer |
| Lab 4 | Binary Semaphores | event signaling, blocked task behavior |
| Lab 5 | Mutexes | shared resource protection, priority inversion awareness |
| Lab 6 | Task Notifications | lightweight task signaling |
| Lab 7 | Software Timers | one-shot timers, periodic timers, deferred actions |
| Lab 8 | Interrupts with RTOS | `FromISR` APIs, ISR-to-task signaling |
| Lab 9 | UART CLI | command parsing, runtime debug interface |

### Hardware Integration and System Design

| Lab | Topic | Concepts |
|---|---|---|
| Lab 10 | SPI Accelerometer Integration | ADXL345 driver, acquisition task, queue-based processing |
| Lab 11 | I2C TOF Sensor Integration | I2C bus driver, proximity/range sensor driver, periodic sampling |
| Lab 12 | Multi-Sensor RTOS Application | SPI + I2C telemetry, mixed-rate tasks, modular architecture |
| Lab 13 | System Health and Reliability | stack/heap monitoring, watchdog concepts, fault handling |

---

## Current Focus

The current hardware integration path is centered on building a small drone/robotics-style telemetry node:

```text
SPI accelerometer task      ┐
                            ├── sensor queue → processing/logging task → UART output
I2C distance sensor task    ┘
```

## Deliverable

Lab 13 is implemented as `labs/health_monitor_sensor_fusion`.

It uses the sensor fusion features:
- TOF and Accelerometer sensor data acquisition
- Sensor data processing and UART transmission

And extends the sensor fusion demo with:
- stack high-water mark reporting
- heap and uptime reporting
- queue space monitoring
- event-group task liveness bits
- IWDG watchdog refresh only when all monitored tasks check in
