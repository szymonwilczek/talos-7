/*
 * test runner - main file running all unit tests
 *
 * cd build && cmake .. && make
 * ./run_tests
 */

#include "unity/unity.h"
#include <stdio.h>

extern void run_macro_config_tests(void);
extern void run_hardware_interface_tests(void);
extern void run_exec_midi_tests(void);
extern void run_cdc_cmd_write_tests(void);

int main(void) {
  printf("================================================\n");
  printf("       TALOS 7 FIRMWARE UNIT TESTS\n");
  printf("================================================\n");

  UNITY_BEGIN();

  run_macro_config_tests();
  run_hardware_interface_tests();
  run_exec_midi_tests();
  run_cdc_cmd_write_tests();

  return UNITY_END();
}
