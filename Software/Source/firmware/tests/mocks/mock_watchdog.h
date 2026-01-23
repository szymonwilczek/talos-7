/*
 * mock for hardware/watchdog.h
 */

#ifndef MOCK_WATCHDOG_H
#define MOCK_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

static inline void watchdog_enable(uint32_t delay_ms, bool pause_on_debug) {
  (void)delay_ms;
  (void)pause_on_debug;
}

static inline void watchdog_update(void) {}

static inline bool watchdog_caused_reboot(void) { return false; }

#endif // MOCK_WATCHDOG_H
