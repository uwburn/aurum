# Aurum

Arduino Uno C API

## Motivation

While Arduino is a fantastic environment for learning both electronics and programming, i often wondered what does it take to program and use the Arduino boards outside Arduino IDE, using raw, general purpose tools.

Nowdays there is [Platfrom.io](https://platformio.org/), that allows you to use Visual Studio Code, but again, it masks some parts of the development process, which is convenient, but takes away part of the learning.

Furthermore, i always wondered why Arduino adopted the more complex C++, while for both MCU programming and learning i feel plain C was more suited.

Given all of this, i tried making some small projects using plain C + avr-gcc + avrdude and a Makefile. After a few experimentations with plain avr-libc i was really missing the usual Arduino functions to perform basic tasks, as `digitalRead`, `digitalWrite`, `tone`, etc. So i started working on a porting of the functions i was using more and bootstrapped this library project with CMake, so that it was easy to include it in application projects.

In the end i ported all the Arduino API, including the core libraries supported by the Uno.

I then moved to add some support for cooperative and preemptive tasks, as i was interested in looking into the details of how a basic RTOS could be implemented.

## Limitations

__This is a "toy" project, and should be treated as that.__

__At the moment, the library supports only the Uno board.__ Actually i have a Rev3 (not R3) board, but as long as the CPU is the ATMega328P it should work.

I limtied myself to the Uno for two reasons:

- I don't have other boards to test
- I didn't want to fill the code with `#ifdef` to cover different hardware configurations

Most of the API is a 1:1 porting of the original Arduino code (__all credits to them__).

Some parts were implemented with the help of LLMs, as my assembly knowledge is very limited and this is by far the most complex stuff i worked on with C.

Probably many projects from the Arduino starter kit are doable using this library. The main limitation is that being a complete rewrite in pure C, this library doesn't support existing Arduino libraries (such as the ones for driving LCDs, motors, specific I2C or SPI devices and so on). So if you need thoose, you have to port them as well.

For some small projects i have been able to use it successfully.

## Features

As in plain C there are no classes and so no private fields, some compromise had to be taken in exposing part of the internals in the public API or use `malloc` to return handles to opaque structs.

As `malloc` should really be avoided in programming an MCU with 2 kb of RAM, i went for leaving parts of the internals visible in public structs.

## How to use

Ensure you have installed AVR toolchain, e.g. on Ubuntu/Debian:

```
sudo apt install gcc-avr avr-libc binutils-avr avrdude
```

You will also need CMake:

```
sudo apt install cmake
```

The recommended workflow to use the library is:

- Create a git repository for you project
- Create the typical C project folder structure:

  ```
  include/
  lib/
  src/
    main.c
  .gitignore
  ```

- Add the library as a git submodule:

  ```
  git submodule add git@github.com:uwburn/aurum.git lib/aurum
  ```

- Create a CMakeLists.txt file like:

  ```
  cmake_minimum_required(VERSION 3.20)

  set(MCU atmega328p)
  set(F_CPU 16000000UL)

  set(CMAKE_C_COMPILER avr-gcc)
  set(CMAKE_ASM_COMPILER avr-gcc)
  set(CMAKE_OBJCOPY avr-objcopy)

  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_STANDARD_REQUIRED ON)

  set(PORT /dev/ttyACM0)

  project(example C ASM)

  add_subdirectory(lib/aurum)

  add_executable(${PROJECT_NAME}
      src/main.c
  )

  target_link_libraries(${PROJECT_NAME}
      PRIVATE
          aurum
  )

  target_include_directories(${PROJECT_NAME} PRIVATE
      include
  )

  target_compile_options(${PROJECT_NAME} PRIVATE
      -mmcu=${MCU}
      -DF_CPU=${F_CPU}
      -Os
      -ffunction-sections
      -fdata-sections
      -Wall
      -Wextra
  )

  target_link_options(${PROJECT_NAME} PRIVATE
      -mmcu=${MCU}
      -Wl,--gc-sections
      -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
  )

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_OBJCOPY}
          -O ihex
          -R .eeprom
          $<TARGET_FILE:${PROJECT_NAME}>
          ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
      COMMENT "Generating ${PROJECT_NAME}.hex"
  )

  add_custom_target(flash
      COMMAND avrdude
          -p ${MCU}
          -c arduino
          -P ${PORT}
          -b 115200
          -U flash:w:${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex:i
      DEPENDS ${PROJECT_NAME}
      COMMENT "Flashing ${MCU}"
  )
  ```

- In your main function, be sure to call `au_init()` from `aurum/core.h` at the beginning, e.g.:

  ```
  void setup();
  void loop();

  int main(void) {
    au_init();
    
    setup();
  
    for (;;) {
      loop();
    }
    
    return 0;
  }
  ```

## Modules

The library is divided into several header files, defining modules for different parts logically grouped together:

- `core.h`: main hardware abstraction layer functions, cover most of the original `Arduino.h` (excluding tone functions)
- `tone.h`: functions to produce tones with a piezo speaker
- `eeprom.h`: functions to perform read/write operations on the EEPROM
- `software_serial.h`: functions and structs to use regular pins as a serial interface
- `wire.h`: functions and structs to perform Wire/I2C interfacing
- `spi.h`: functions and structs to perform SPI interfacing
- `c_task.h`: functions and structs to implement cooperative task scheduling
- `p_task.h`: functions and structs to implement preemptive task scheduling
- `print.h`: mimic the Print class from Arduino, it's included directly in `core.h`

## Disclaimer

I don't want to be held responsible for bricked boards or damaged peripherals!

As clearly stated this is an experimental project, use at your own risk and double check what you're doing.

Have fun!

## Credits

- Arduino team and community, for the boards and the original source code used as a reference
- Glutio [TaskFun](https://github.com/glutio/Taskfun), for some ideas on how to deal with preemptive tasks