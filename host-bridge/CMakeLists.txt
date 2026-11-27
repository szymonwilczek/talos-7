cmake_minimum_required(VERSION 3.16)
project(talos_bridge C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror -pedantic)
add_compile_definitions(_POSIX_C_SOURCE=200809L _XOPEN_SOURCE=700)

add_library(talos_bridge_core STATIC
    src/bridge_daemon.c
    src/bridge_idle.c
    src/bridge_monitors.c
    src/bridge_serial.c
    src/bridge_socket.c
    src/bridge_state.c
)

target_include_directories(talos_bridge_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(talos-bridge
    src/main.c
)
target_link_libraries(talos-bridge PRIVATE talos_bridge_core)

add_executable(talos-evt
    src/talos_evt.c
)
target_link_libraries(talos-evt PRIVATE talos_bridge_core)

enable_testing()
add_subdirectory(tests)
