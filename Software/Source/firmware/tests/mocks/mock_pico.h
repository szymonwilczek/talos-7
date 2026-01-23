/*
 * mock for Pico SDK - types definitions and functions for host
 */

#ifndef MOCK_PICO_H
#define MOCK_PICO_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// pico types
typedef unsigned int uint;

// time functions (mock)
static inline uint32_t time_us_32(void) { return 0; }
static inline void sleep_ms(uint32_t ms) { (void)ms; }
static inline void sleep_us(uint64_t us) { (void)us; }

// printf wrapper
#define printf(...) printf(__VA_ARGS__)

#endif // MOCK_PICO_H
