/*
 * Unity Test Framework
 * Copyright (c) 2007-2021 Mike Karlesky, Mark VanderVoord, Greg Williams
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef UNITY_H
#define UNITY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// test counters
extern int Unity_TestsRun;
extern int Unity_TestsFailed;
extern int Unity_TestsIgnored;
extern const char *Unity_CurrentTest;

// initialize Unity
#define UNITY_BEGIN()                                                          \
  Unity_TestsRun = 0;                                                          \
  Unity_TestsFailed = 0;                                                       \
  Unity_TestsIgnored = 0

// run a test function
#define RUN_TEST(func)                                                         \
  do {                                                                         \
    Unity_CurrentTest = #func;                                                 \
    printf("%s:%d:%s:", __FILE__, __LINE__, #func);                            \
    Unity_TestsRun++;                                                          \
    func();                                                                    \
    if (Unity_TestsFailed == Unity_TestsRun - 1) {                             \
      printf("PASS\n");                                                        \
    }                                                                          \
  } while (0)

// end Unity, return result (returns failure count)
#define UNITY_END()                                                            \
  (printf("\n-----------------------\n"),                                      \
   printf("%d Tests %d Failures %d Ignored\n", Unity_TestsRun,                 \
          Unity_TestsFailed, Unity_TestsIgnored),                              \
   (Unity_TestsFailed == 0) ? printf("OK\n") : printf("FAIL\n"),               \
   Unity_TestsFailed)

// assertion macros
#define TEST_FAIL_MESSAGE(msg)                                                 \
  do {                                                                         \
    printf("FAIL\n  %s\n", msg);                                               \
    Unity_TestsFailed++;                                                       \
    return;                                                                    \
  } while (0)

#define TEST_ASSERT(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      TEST_FAIL_MESSAGE("Assertion failed: " #condition);                      \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_EQUAL(expected, actual)                                    \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      printf("FAIL\n  Expected %d but was %d\n", (int)(expected),              \
             (int)(actual));                                                   \
      Unity_TestsFailed++;                                                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL_INT(expected, actual)                                \
  TEST_ASSERT_EQUAL(expected, actual)
#define TEST_ASSERT_EQUAL_UINT8(expected, actual)                              \
  TEST_ASSERT_EQUAL(expected, actual)
#define TEST_ASSERT_EQUAL_UINT16(expected, actual)                             \
  TEST_ASSERT_EQUAL(expected, actual)
#define TEST_ASSERT_EQUAL_UINT32(expected, actual)                             \
  TEST_ASSERT_EQUAL(expected, actual)

#define TEST_ASSERT_NOT_EQUAL(expected, actual)                                \
  do {                                                                         \
    if ((expected) == (actual)) {                                              \
      printf("FAIL\n  Expected NOT %d but was %d\n", (int)(expected),          \
             (int)(actual));                                                   \
      Unity_TestsFailed++;                                                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                             \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      printf("FAIL\n  Expected \"%s\" but was \"%s\"\n", (expected),           \
             (actual));                                                        \
      Unity_TestsFailed++;                                                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NULL(ptr)                                                  \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      TEST_FAIL_MESSAGE("Expected NULL but was not NULL");                     \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NOT_NULL(ptr)                                              \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      TEST_FAIL_MESSAGE("Expected not NULL but was NULL");                     \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual)                            \
  do {                                                                         \
    if ((actual) <= (threshold)) {                                             \
      printf("FAIL\n  Expected > %d but was %d\n", (int)(threshold),           \
             (int)(actual));                                                   \
      Unity_TestsFailed++;                                                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_LESS_THAN(threshold, actual)                               \
  do {                                                                         \
    if ((actual) >= (threshold)) {                                             \
      printf("FAIL\n  Expected < %d but was %d\n", (int)(threshold),           \
             (int)(actual));                                                   \
      Unity_TestsFailed++;                                                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)                        \
  do {                                                                         \
    if (memcmp((expected), (actual), (len)) != 0) {                            \
      TEST_FAIL_MESSAGE("Memory comparison failed");                           \
    }                                                                          \
  } while (0)

#define TEST_IGNORE()                                                          \
  do {                                                                         \
    printf("IGNORED\n");                                                       \
    Unity_TestsIgnored++;                                                      \
    return;                                                                    \
  } while (0)

#define TEST_IGNORE_MESSAGE(msg)                                               \
  do {                                                                         \
    printf("IGNORED: %s\n", msg);                                              \
    Unity_TestsIgnored++;                                                      \
    return;                                                                    \
  } while (0)

#endif // UNITY_H
