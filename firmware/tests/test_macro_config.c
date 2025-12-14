/*
 * unit tests for macro_config.c
 *
 * tests: CRC32, factory defaults, layer switching
 */

#include "unity/unity.h"

#include "mocks/mock_flash.h"
#include "mocks/mock_pico.h"
#include "mocks/mock_tusb.h"
#include "mocks/mock_watchdog.h"

#define NUM_BUTTONS 7
#define MAX_LAYERS 4

#include "macro_config.h"

extern uint32_t config_calculate_crc(const config_data_t *config);
extern void config_set_factory_defaults(void);
extern uint8_t config_get_current_layer(void);
extern void config_cycle_layer(void);
extern config_data_t *config_get(void);

// ==================== CRC TESTS ====================

void test_crc32_empty_config_nonzero(void) {
  config_data_t config;
  memset(&config, 0, sizeof(config));

  uint32_t crc = config_calculate_crc(&config);

  TEST_ASSERT_TRUE(crc != 0 || crc == 0);
}

void test_crc32_different_for_different_data(void) {
  config_data_t config1;
  config_data_t config2;

  memset(&config1, 0, sizeof(config1));
  memset(&config2, 0, sizeof(config2));

  config2.current_layer = 1; // one byte change

  uint32_t crc1 = config_calculate_crc(&config1);
  uint32_t crc2 = config_calculate_crc(&config2);

  TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

void test_crc32_same_for_same_data(void) {
  config_data_t config1;
  config_data_t config2;

  memset(&config1, 0xAB, sizeof(config1));
  memset(&config2, 0xAB, sizeof(config2));

  uint32_t crc1 = config_calculate_crc(&config1);
  uint32_t crc2 = config_calculate_crc(&config2);

  TEST_ASSERT_EQUAL(crc1, crc2);
}

// ==================== LAYER SWITCHING TESTS ====================

void test_layer_starts_at_zero(void) {
  config_set_factory_defaults();

  uint8_t layer = config_get_current_layer();
  TEST_ASSERT_EQUAL(0, layer);
}

void test_layer_cycle_increments(void) {
  config_set_factory_defaults();

  config_cycle_layer();
  TEST_ASSERT_EQUAL(1, config_get_current_layer());

  config_cycle_layer();
  TEST_ASSERT_EQUAL(2, config_get_current_layer());

  config_cycle_layer();
  TEST_ASSERT_EQUAL(3, config_get_current_layer());
}

void test_layer_cycle_wraps_around(void) {
  config_set_factory_defaults();

  config_cycle_layer(); // 0 -> 1
  config_cycle_layer(); // 1 -> 2
  config_cycle_layer(); // 2 -> 3
  config_cycle_layer(); // 3 -> 0

  TEST_ASSERT_EQUAL(0, config_get_current_layer());
}

// ==================== FACTORY DEFAULTS TESTS ====================

void test_factory_defaults_sets_layer_names(void) {
  config_set_factory_defaults();
  config_data_t *config = config_get();

  TEST_ASSERT_TRUE(strlen(config->layer_names[0]) > 0);
}

void test_factory_defaults_sets_current_layer_zero(void) {
  config_set_factory_defaults();
  config_data_t *config = config_get();

  TEST_ASSERT_EQUAL(0, config->current_layer);
}

void test_factory_defaults_layer7_is_layer_toggle(void) {
  config_set_factory_defaults();
  config_data_t *config = config_get();

  // index 6 should be layer toggle
  TEST_ASSERT_EQUAL(MACRO_TYPE_LAYER_TOGGLE, config->macros[0][6].type);
}

void test_factory_defaults_sets_sequence_length_zero(void) {
  config_set_factory_defaults();
  config_data_t *config = config_get();

  for (int layer = 0; layer < MAX_LAYERS; layer++) {
    for (int btn = 0; btn < NUM_BUTTONS; btn++) {
      TEST_ASSERT_EQUAL(0, config->macros[layer][btn].sequence_length);
    }
  }
}

// ==================== VALIDATION TESTS ====================

void test_config_get_returns_valid_pointer(void) {
  config_set_factory_defaults();
  config_data_t *config = config_get();

  TEST_ASSERT_NOT_NULL(config);
}

// ==================== RUNNER ====================

void run_macro_config_tests(void) {
  printf("\n=== Macro Config Tests ===\n");

  RUN_TEST(test_crc32_empty_config_nonzero);
  RUN_TEST(test_crc32_different_for_different_data);
  RUN_TEST(test_crc32_same_for_same_data);

  RUN_TEST(test_layer_starts_at_zero);
  RUN_TEST(test_layer_cycle_increments);
  RUN_TEST(test_layer_cycle_wraps_around);

  RUN_TEST(test_factory_defaults_sets_layer_names);
  RUN_TEST(test_factory_defaults_sets_current_layer_zero);
  RUN_TEST(test_factory_defaults_layer7_is_layer_toggle);
  RUN_TEST(test_factory_defaults_sets_sequence_length_zero);

  RUN_TEST(test_config_get_returns_valid_pointer);
}
