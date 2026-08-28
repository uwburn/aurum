cmake_minimum_required(VERSION 3.16)

project(aurum LANGUAGES C)

# OBJECT

# add_library(aurum OBJECT
#     src/aurum.c
# )

# target_include_directories(aurum
#     PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}/include
# )


# INTERFACE + SOURCES

add_library(aurum INTERFACE)

target_sources(aurum
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/aurum.c
)

target_include_directories(aurum
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)