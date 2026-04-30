cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w CACHE STRING "Target board")

include(pico_sdk_import.cmake)

project(pico_2w_clock C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# WIZnet Ethernet HAT configuration
set(WIZNET_ROOT ${CMAKE_CURRENT_LIST_DIR}/external/WIZnet-PICO-C)
set(WIZNET_DIR ${WIZNET_ROOT}/libraries/ioLibrary_Driver)
set(PORT_DIR ${WIZNET_ROOT}/port)
set(BOARD_NAME WIZnet_Ethernet_HAT)
set(WIZNET_CHIP W5100S)

add_compile_definitions(_WIZCHIP_=W5100S)
add_compile_definitions(DEVICE_BOARD_NAME=WIZnet_Ethernet_HAT)

add_subdirectory(${WIZNET_ROOT}/libraries ${CMAKE_BINARY_DIR}/wiznet_libraries)
add_subdirectory(${WIZNET_ROOT}/port ${CMAKE_BINARY_DIR}/wiznet_port)

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

add_library(wiznet_ethernet
    src/wiznet_ethernet.c
)

target_include_directories(wiznet_ethernet
    PUBLIC
        include
)

target_link_libraries(wiznet_ethernet
    PUBLIC
        pico_stdlib
        pico_unique_id
        hardware_spi
        hardware_dma
        ETHERNET_FILES
        IOLIBRARY_FILES
        DHCP_FILES
        TIMER_FILES
)

add_executable(pico_2w_clock
    src/main.c
)

target_link_libraries(pico_2w_clock
    PRIVATE
        pico_stdlib
        pico_8seg
        wiznet_ethernet
)

pico_enable_stdio_usb(pico_2w_clock 1)
pico_enable_stdio_uart(pico_2w_clock 0)

pico_add_extra_outputs(pico_2w_clock)