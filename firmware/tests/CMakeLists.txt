#   mkdir build && cd build
#   cmake ..
#   make
#   ./run_tests

cmake_minimum_required(VERSION 3.13)
project(talos7_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# unity framework
add_library(unity STATIC
    unity/unity.c
)
target_include_directories(unity PUBLIC unity)

# mocks
add_library(mocks STATIC
    mocks/mocks.c
)
target_include_directories(mocks PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# test executable
add_executable(run_tests
    test_runner.c
    test_macro_config.c
    test_hardware_interface.c
    test_exec_midi.c
    test_cdc_cmd_write.c
)

target_include_directories(run_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(run_tests unity mocks)

# CTest integration
enable_testing()
add_test(NAME unit_tests COMMAND run_tests)

add_custom_target(test_all
    COMMAND ./run_tests
    DEPENDS run_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all unit tests..."
)
