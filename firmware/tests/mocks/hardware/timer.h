#ifndef MOCK_HARDWARE_TIMER_H
#define MOCK_HARDWARE_TIMER_H

#include <stdbool.h>
#include <stdint.h>

struct repeating_timer {
	int dummy;
};

typedef bool (*repeating_timer_callback_t)(struct repeating_timer *t);

static inline bool add_repeating_timer_ms(int32_t delay_ms,
					  repeating_timer_callback_t cb,
					  void *user_data,
					  struct repeating_timer *out)
{
	(void)delay_ms;
	(void)cb;
	(void)user_data;
	(void)out;
	return true;
}

static inline bool cancel_repeating_timer(struct repeating_timer *t)
{
	(void)t;
	return true;
}

#endif // MOCK_HARDWARE_TIMER_H
