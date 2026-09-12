cmake_minimum_required(VERSION 3.16)

project(aurum LANGUAGES C ASM)

# OBJECT

# set(MCU atmega328p)
# set(F_CPU 16000000UL)

# set(CMAKE_C_COMPILER avr-gcc)

# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# add_library(aurum OBJECT
#     src/system.c
#     src/digital_io.c
#     src/analog_io.c
#     src/time.c
#     src/serial.c
#     src/ext_interrupts.c
#     src/tone.c
#     src/pulse.s
#     src/pulse.c
#     src/shift.c
#     src/eeprom.c
#     src/software_serial.c
# )

# target_include_directories(aurum
#     PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}/include
# )

# target_include_directories(${PROJECT_NAME} PRIVATE
#     include
# )

# target_compile_options(${PROJECT_NAME} PRIVATE
#     -mmcu=${MCU}
#     -DF_CPU=${F_CPU}
#     -Os
#     -ffunction-sections
#     -fdata-sections
#     -Wall
#     -Wextra
# )

# target_link_options(${PROJECT_NAME} PRIVATE
#     -mmcu=${MCU}
# )


# INTERFACE + SOURCES

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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/r_task.c
)

target_include_directories(aurum
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)