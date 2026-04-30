cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w CACHE STRING "Target board")

include(pico_sdk_import.cmake)

project(pico_2w_clock C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_library(pico_8seg
    src/pico_8seg.c
)

target_include_directories(pico_8seg
    PUBLIC
        include
)

target_link_libraries(pico_8seg
    PUBLIC
        pico_stdlib
)

add_executable(pico_2w_clock
    src/main.c
)

target_link_libraries(pico_2w_clock
    PRIVATE
        pico_stdlib
        pico_8seg
)

pico_enable_stdio_usb(pico_2w_clock 1)
pico_enable_stdio_uart(pico_2w_clock 0)

pico_add_extra_outputs(pico_2w_clock)
