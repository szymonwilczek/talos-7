#ifndef MOCK_PICO_STDLIB_SHIM_H
#define MOCK_PICO_STDLIB_SHIM_H

#include "mock_pico.h"
#include "mock_companion_hw.h"
#include "mock_gpio.h"

typedef uint64_t absolute_time_t;

static inline absolute_time_t get_absolute_time(void)
{
	return (absolute_time_t)mock_time_ms * 1000ULL;
}

static inline uint32_t to_ms_since_boot(absolute_time_t t)
{
	return (uint32_t)(t / 1000ULL);
}

static inline void tight_loop_contents(void)
{
}

#endif // MOCK_PICO_STDLIB_SHIM_H
