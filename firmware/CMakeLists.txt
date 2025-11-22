cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Pico SDK
include(pico_sdk_import.cmake)

project(macrokeyboard C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(macrokeyboard
    src/main.c
    src/macro_config.c
    src/usb_descriptors.c
    src/mock_hardware.c
    src/hardware_interface.c
    src/executor/macro_executor.c
    src/executor/actions/exec_hid_core.c
    src/executor/actions/exec_mouse.c
    src/executor/actions/exec_script.c
    src/executor/actions/exec_text.c
    src/oled_display.c
    src/cdc/commands/cdc_cmd_read.c
    src/cdc/commands/cdc_cmd_write.c
    src/cdc/commands/cdc_cmd_system.c
    src/cdc/cdc_dispatcher.c
    src/cdc/cdc_transport.c
)

target_include_directories(macrokeyboard PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(macrokeyboard
    pico_stdlib
    pico_unique_id
    hardware_flash
    hardware_sync
    hardware_i2c
    hardware_gpio
    hardware_spi
    tinyusb_device
    tinyusb_board
)

# USB output
pico_enable_stdio_usb(macrokeyboard 0)
pico_enable_stdio_uart(macrokeyboard 0)

pico_add_extra_outputs(macrokeyboard)
