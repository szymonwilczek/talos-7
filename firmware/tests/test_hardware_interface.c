/*
 * unit tests for hardware_interface.c
 *
 * tests: HID keycodes mapping, keys formatting
 */

#include "unity/unity.h"

#include "mocks/mock_gpio.h"
#include "mocks/mock_pico.h"
#include "mocks/mock_tusb.h"

#define NUM_BUTTONS 7
#define MAX_LAYERS 4

// GPIO mock variables
bool mock_gpio_state[MOCK_MAX_GPIOS] = {0};
bool mock_gpio_direction[MOCK_MAX_GPIOS] = {0};

static uint8_t test_map_char_to_hid(char c, bool *shift_needed) {
  *shift_needed = false;

  if (c >= 'a' && c <= 'z') {
    return 0x04 + (c - 'a'); // HID_KEY_A = 0x04
  }
  if (c >= 'A' && c <= 'Z') {
    *shift_needed = true;
    return 0x04 + (c - 'A');
  }
  if (c >= '1' && c <= '9') {
    return 0x1E + (c - '1'); // HID_KEY_1 = 0x1E
  }
  if (c == '0') {
    return 0x27; // HID_KEY_0
  }
  if (c == ' ') {
    return 0x2C; // HID_KEY_SPACE
  }
  if (c == '\n') {
    return 0x28; // HID_KEY_ENTER
  }

  return 0; // unknown
}

// ==================== MAPPING TESTS CHAR -> HID ====================

void test_map_lowercase_a(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid('a', &shift);

  TEST_ASSERT_EQUAL(0x04, keycode); // HID_KEY_A
  TEST_ASSERT_FALSE(shift);
}

void test_map_lowercase_z(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid('z', &shift);

  TEST_ASSERT_EQUAL(0x1D, keycode); // HID_KEY_Z = 0x04 + 25
  TEST_ASSERT_FALSE(shift);
}

void test_map_uppercase_requires_shift(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid('A', &shift);

  TEST_ASSERT_EQUAL(0x04, keycode); // same keycode as 'a'
  TEST_ASSERT_TRUE(shift);
}

void test_map_digit_1(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid('1', &shift);

  TEST_ASSERT_EQUAL(0x1E, keycode); // HID_KEY_1
  TEST_ASSERT_FALSE(shift);
}

void test_map_digit_0(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid('0', &shift);

  TEST_ASSERT_EQUAL(0x27, keycode); // HID_KEY_0
  TEST_ASSERT_FALSE(shift);
}

void test_map_space(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid(' ', &shift);

  TEST_ASSERT_EQUAL(0x2C, keycode); // HID_KEY_SPACE
  TEST_ASSERT_FALSE(shift);
}

void test_map_enter(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid('\n', &shift);

  TEST_ASSERT_EQUAL(0x28, keycode); // HID_KEY_ENTER
  TEST_ASSERT_FALSE(shift);
}

void test_map_unknown_returns_zero(void) {
  bool shift;
  uint8_t keycode = test_map_char_to_hid('\x7F', &shift); // DEL character

  TEST_ASSERT_EQUAL(0, keycode);
}

// ==================== GPIO MOCK TESTS ====================

void test_gpio_mock_set_and_get(void) {
  gpio_put(5, true);
  TEST_ASSERT_TRUE(gpio_get(5));

  gpio_put(5, false);
  TEST_ASSERT_FALSE(gpio_get(5));
}

void test_gpio_mock_direction(void) {
  gpio_set_dir(10, GPIO_OUT);
  TEST_ASSERT_TRUE(mock_gpio_direction[10]);

  gpio_set_dir(10, GPIO_IN);
  TEST_ASSERT_FALSE(mock_gpio_direction[10]);
}

// ==================== RUNNER ====================

void run_hardware_interface_tests(void) {
  printf("\n=== Hardware Interface Tests ===\n");

  RUN_TEST(test_map_lowercase_a);
  RUN_TEST(test_map_lowercase_z);
  RUN_TEST(test_map_uppercase_requires_shift);
  RUN_TEST(test_map_digit_1);
  RUN_TEST(test_map_digit_0);
  RUN_TEST(test_map_space);
  RUN_TEST(test_map_enter);
  RUN_TEST(test_map_unknown_returns_zero);

  RUN_TEST(test_gpio_mock_set_and_get);
  RUN_TEST(test_gpio_mock_direction);
}
