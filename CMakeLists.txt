cmake_minimum_required(VERSION 3.16)

project(aurum VERSION 1.0.0 LANGUAGES C ASM)

add_library(aurum INTERFACE)

target_sources(aurum
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pins.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/digital_io.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/analog_io.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/time.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ext_interrupts.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tone.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pulse.s
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pulse.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shift.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/eeprom.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/software_serial.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/twi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/wire.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/print.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/c_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/task_context.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/task_context.s
        ${CMAKE_CURRENT_SOURCE_DIR}/src/p_task.c
)

target_include_directories(aurum
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)