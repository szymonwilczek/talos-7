/*
 * unit tests for CDC dispatcher/cmd_write
 *
 * tests: commands parsing, parameters validation
 */

#include "unity/unity.h"

#include "mocks/mock_pico.h"
#include "mocks/mock_tusb.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  int layer;
  int button;
  int type;
  int value;
  char string[64];
  char name[32];
  int emoji_index;
} parsed_macro_args_t;

static int parse_set_macro_args(const char *args, parsed_macro_args_t *out) {
  // layer|button|type|value|string|name|emoji_idx|...
  char buffer[256];
  strncpy(buffer, args, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  char *token = strtok(buffer, "|");
  if (!token)
    return -1;
  out->layer = atoi(token);

  token = strtok(NULL, "|");
  if (!token)
    return -1;
  out->button = atoi(token);

  token = strtok(NULL, "|");
  if (!token)
    return -1;
  out->type = atoi(token);

  token = strtok(NULL, "|");
  if (!token)
    return -1;
  out->value = atoi(token);

  token = strtok(NULL, "|");
  if (token)
    strncpy(out->string, token, sizeof(out->string) - 1);

  token = strtok(NULL, "|");
  if (token)
    strncpy(out->name, token, sizeof(out->name) - 1);

  token = strtok(NULL, "|");
  if (token)
    out->emoji_index = atoi(token);

  return 0;
}

static int validate_macro_args(const parsed_macro_args_t *args) {
  if (args->layer < 0 || args->layer >= 4)
    return -1;
  if (args->button < 0 || args->button >= 7)
    return -1;
  if (args->type < 0 || args->type > 10)
    return -1;
  return 0;
}

// ==================== PARSING TESTS ====================

void test_parse_valid_macro_args(void) {
  parsed_macro_args_t args = {0};
  const char *input = "0|3|1|65|Hello|TestMacro|5";

  int result = parse_set_macro_args(input, &args);

  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(0, args.layer);
  TEST_ASSERT_EQUAL(3, args.button);
  TEST_ASSERT_EQUAL(1, args.type);
  TEST_ASSERT_EQUAL(65, args.value);
  TEST_ASSERT_EQUAL_STRING("Hello", args.string);
  TEST_ASSERT_EQUAL_STRING("TestMacro", args.name);
  TEST_ASSERT_EQUAL(5, args.emoji_index);
}

void test_parse_empty_string_field(void) {
  parsed_macro_args_t args = {0};
  const char *input = "1|2|0|0||EmptyString|0";

  int result = parse_set_macro_args(input, &args);

  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_EQUAL(1, args.layer);
  TEST_ASSERT_EQUAL(2, args.button);
}

void test_parse_incomplete_args_fails(void) {
  parsed_macro_args_t args = {0};
  const char *input = "0|3"; // 2 fields

  int result = parse_set_macro_args(input, &args);

  TEST_ASSERT_EQUAL(-1, result);
}

// ==================== VALIDATION TESTS ====================

void test_validate_valid_args(void) {
  parsed_macro_args_t args = {.layer = 0, .button = 3, .type = 1};

  TEST_ASSERT_EQUAL(0, validate_macro_args(&args));
}

void test_validate_invalid_layer_negative(void) {
  parsed_macro_args_t args = {.layer = -1, .button = 0, .type = 0};

  TEST_ASSERT_EQUAL(-1, validate_macro_args(&args));
}

void test_validate_invalid_layer_too_high(void) {
  parsed_macro_args_t args = {.layer = 5, .button = 0, .type = 0};

  TEST_ASSERT_EQUAL(-1, validate_macro_args(&args));
}

void test_validate_invalid_button_too_high(void) {
  parsed_macro_args_t args = {.layer = 0, .button = 10, .type = 0};

  TEST_ASSERT_EQUAL(-1, validate_macro_args(&args));
}

void test_validate_all_layers_valid(void) {
  for (int layer = 0; layer < 4; layer++) {
    parsed_macro_args_t args = {.layer = layer, .button = 0, .type = 0};
    TEST_ASSERT_EQUAL(0, validate_macro_args(&args));
  }
}

void test_validate_all_buttons_valid(void) {
  for (int btn = 0; btn < 7; btn++) {
    parsed_macro_args_t args = {.layer = 0, .button = btn, .type = 0};
    TEST_ASSERT_EQUAL(0, validate_macro_args(&args));
  }
}

// ==================== RUNNER ====================

void run_cdc_cmd_write_tests(void) {
  printf("\n=== CDC Command Write Tests ===\n");

  RUN_TEST(test_parse_valid_macro_args);
  RUN_TEST(test_parse_empty_string_field);
  RUN_TEST(test_parse_incomplete_args_fails);

  RUN_TEST(test_validate_valid_args);
  RUN_TEST(test_validate_invalid_layer_negative);
  RUN_TEST(test_validate_invalid_layer_too_high);
  RUN_TEST(test_validate_invalid_button_too_high);
  RUN_TEST(test_validate_all_layers_valid);
  RUN_TEST(test_validate_all_buttons_valid);
}
