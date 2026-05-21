cmake_minimum_required(VERSION 3.20)

project(FreeRTOS_Zephyr_Labs C ASM)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(LAB_NAME "health_monitor_sensor_fusion" CACHE STRING "Lab to build")

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

set(LAB_MAIN
    ${CMAKE_SOURCE_DIR}/labs/${LAB_NAME}/src/main.c
)

add_executable(${PROJECT_NAME}.elf
    ${DEVICE_SOURCES}
    ${COMMON_SOURCES}
    ${FREERTOS_SOURCES}
    ${LAB_MAIN}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${CMAKE_SOURCE_DIR}/common/inc
    ${CMAKE_SOURCE_DIR}/common/drivers
    ${CMAKE_SOURCE_DIR}/cmsis/core
    ${CMAKE_SOURCE_DIR}/cmsis/device/stm32f4xx
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F401xE
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)
