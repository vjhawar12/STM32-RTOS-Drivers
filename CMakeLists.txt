cmake_minimum_required(VERSION 3.20)

project(STM32_Vehicle_Telemetry C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# The product-level telemetry application is the default build. Existing labs
# remain available through -DBUILD_LAB=ON -DLAB_NAME=<directory>.
option(BUILD_LAB "Build one of the standalone FreeRTOS labs" OFF)
set(APP_NAME "vehicle_telemetry" CACHE STRING "Application under apps/ to build")
set(LAB_NAME "periodic_tasks" CACHE STRING "Lab under labs/ to build when BUILD_LAB=ON")

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_compile_options(
    ${MCU_FLAGS}
    -Wall
    -Wextra
    -Wshadow
    -Wundef
    -ffunction-sections
    -fdata-sections
    -fno-common
)

add_link_options(
    ${MCU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/linker/STM32F401RE_FLASH.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
    --specs=nosys.specs
    --specs=nano.specs
)

set(DEVICE_SOURCES
    ${CMAKE_SOURCE_DIR}/cmsis/device/stm32f4xx/system_stm32f4xx.c
    ${CMAKE_SOURCE_DIR}/cmsis/device/stm32f4xx/startup_stm32f401xe.s
)

set(COMMON_SOURCES
    ${CMAKE_SOURCE_DIR}/common/src/platform.c
    ${CMAKE_SOURCE_DIR}/common/src/exception.c
    ${CMAKE_SOURCE_DIR}/common/drivers/spi.c
    ${CMAKE_SOURCE_DIR}/common/drivers/adxl345.c
    ${CMAKE_SOURCE_DIR}/common/drivers/i2c.c
    ${CMAKE_SOURCE_DIR}/common/drivers/vl6180.c
    ${CMAKE_SOURCE_DIR}/common/drivers/uart.c
)

set(FREERTOS_SOURCES
    ${CMAKE_SOURCE_DIR}/freertos/list.c
    ${CMAKE_SOURCE_DIR}/freertos/queue.c
    ${CMAKE_SOURCE_DIR}/freertos/tasks.c
    ${CMAKE_SOURCE_DIR}/freertos/heap_4.c
    ${CMAKE_SOURCE_DIR}/freertos/port.c
    ${CMAKE_SOURCE_DIR}/freertos/syscalls.c
    ${CMAKE_SOURCE_DIR}/freertos/timers.c
    ${CMAKE_SOURCE_DIR}/freertos/event_groups.c
)

if(BUILD_LAB)
    set(ENTRY_SOURCE ${CMAKE_SOURCE_DIR}/labs/${LAB_NAME}/src/main.c)
    set(FIRMWARE_TARGET ${LAB_NAME})
else()
    set(ENTRY_SOURCE ${CMAKE_SOURCE_DIR}/apps/${APP_NAME}/src/main.c)
    set(FIRMWARE_TARGET ${APP_NAME})
endif()

if(NOT EXISTS ${ENTRY_SOURCE})
    message(FATAL_ERROR "Selected firmware entry point does not exist: ${ENTRY_SOURCE}")
endif()

add_executable(${FIRMWARE_TARGET}.elf
    ${DEVICE_SOURCES}
    ${COMMON_SOURCES}
    ${FREERTOS_SOURCES}
    ${ENTRY_SOURCE}
)

target_include_directories(${FIRMWARE_TARGET}.elf PRIVATE
    ${CMAKE_SOURCE_DIR}/common/inc
    ${CMAKE_SOURCE_DIR}/common/drivers
    ${CMAKE_SOURCE_DIR}/cmsis/core
    ${CMAKE_SOURCE_DIR}/cmsis/device/stm32f4xx
)

target_compile_definitions(${FIRMWARE_TARGET}.elf PRIVATE
    STM32F401xE
)

add_custom_command(TARGET ${FIRMWARE_TARGET}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${FIRMWARE_TARGET}.elf>
            ${CMAKE_BINARY_DIR}/${FIRMWARE_TARGET}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${FIRMWARE_TARGET}.elf>
            ${CMAKE_BINARY_DIR}/${FIRMWARE_TARGET}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${FIRMWARE_TARGET}.elf>
)
