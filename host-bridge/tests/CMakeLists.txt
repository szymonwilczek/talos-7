add_executable(bridge_tests
    test_runner.c
    test_bridge_ci.c
    test_bridge_state.c
    test_bridge_serial.c
    test_bridge_monitors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/tests/unity/unity.c
)

target_include_directories(bridge_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/tests/unity
)

target_link_libraries(bridge_tests PRIVATE
    talos_bridge_core
)

add_test(NAME bridge_tests COMMAND bridge_tests)
