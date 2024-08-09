cmake_minimum_required(VERSION 3.1)
project(msp_example C)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(msp_example main.c msp/msp.c serial/serial.c elrs_backpack/elrs_backpack.c)
