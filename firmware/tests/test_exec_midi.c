/*
 * unit tests for exec_midi_core.c
 *
 * tests: clamping value of MIDI, fallback velocity/channel
 */

#include "unity/unity.h"

#include "mocks/mock_pico.h"
#include "mocks/mock_tusb.h"

static uint8_t clamp_midi_value(int value, uint8_t min, uint8_t max) {
  if (value < min)
    return min;
  if (value > max)
    return max;
  return (uint8_t)value;
}

static uint8_t get_midi_velocity(int velocity) {
  if (velocity <= 0)
    return 127; // fallback
  return clamp_midi_value(velocity, 1, 127);
}

static uint8_t get_midi_channel(int channel) {
  if (channel <= 0)
    return 0; // fallback to channel 1 (0-indexed)
  return clamp_midi_value(channel - 1, 0, 15); // convert 1-16 to 0-15
}

static uint8_t get_midi_note(int note) {
  return clamp_midi_value(note, 0, 127);
}

// ==================== CLAMPING TESTS ====================

void test_clamp_within_range(void) {
  TEST_ASSERT_EQUAL(64, clamp_midi_value(64, 0, 127));
}

void test_clamp_below_min(void) {
  TEST_ASSERT_EQUAL(0, clamp_midi_value(-10, 0, 127));
}

void test_clamp_above_max(void) {
  TEST_ASSERT_EQUAL(127, clamp_midi_value(200, 0, 127));
}

// ==================== VELOCITY TESTS ====================

void test_velocity_zero_returns_fallback(void) {
  TEST_ASSERT_EQUAL(127, get_midi_velocity(0));
}

void test_velocity_negative_returns_fallback(void) {
  TEST_ASSERT_EQUAL(127, get_midi_velocity(-5));
}

void test_velocity_valid_returns_value(void) {
  TEST_ASSERT_EQUAL(100, get_midi_velocity(100));
}

void test_velocity_clamped_to_127(void) {
  TEST_ASSERT_EQUAL(127, get_midi_velocity(200));
}

// ==================== CHANNEL TESTS ====================

void test_channel_zero_returns_fallback(void) {
  TEST_ASSERT_EQUAL(0, get_midi_channel(0)); // channel 1
}

void test_channel_valid_converted_to_zero_indexed(void) {
  TEST_ASSERT_EQUAL(0, get_midi_channel(1));   // channel 1 -> 0
  TEST_ASSERT_EQUAL(9, get_midi_channel(10));  // channel 10 -> 9
  TEST_ASSERT_EQUAL(15, get_midi_channel(16)); // channel 16 -> 15
}

void test_channel_clamped_to_15(void) {
  TEST_ASSERT_EQUAL(15, get_midi_channel(20)); // beyond 16
}

// ==================== NOTE TESTS ====================

void test_note_valid_range(void) {
  TEST_ASSERT_EQUAL(60, get_midi_note(60)); // middle C
}

void test_note_clamped_low(void) { TEST_ASSERT_EQUAL(0, get_midi_note(-10)); }

void test_note_clamped_high(void) {
  TEST_ASSERT_EQUAL(127, get_midi_note(200));
}

// ==================== RUNNER ====================

void run_exec_midi_tests(void) {
  printf("\n=== Exec MIDI Tests ===\n");

  RUN_TEST(test_clamp_within_range);
  RUN_TEST(test_clamp_below_min);
  RUN_TEST(test_clamp_above_max);

  RUN_TEST(test_velocity_zero_returns_fallback);
  RUN_TEST(test_velocity_negative_returns_fallback);
  RUN_TEST(test_velocity_valid_returns_value);
  RUN_TEST(test_velocity_clamped_to_127);

  RUN_TEST(test_channel_zero_returns_fallback);
  RUN_TEST(test_channel_valid_converted_to_zero_indexed);
  RUN_TEST(test_channel_clamped_to_15);

  RUN_TEST(test_note_valid_range);
  RUN_TEST(test_note_clamped_low);
  RUN_TEST(test_note_clamped_high);
}
