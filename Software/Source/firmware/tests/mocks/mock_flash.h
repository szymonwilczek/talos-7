/*
 * mock for hardware/flash.h & hardware/sync.h
 */

#ifndef MOCK_FLASH_H
#define MOCK_FLASH_H

#include <stdint.h>
#include <string.h>

// flash constants
#define FLASH_SECTOR_SIZE 4096
#define FLASH_PAGE_SIZE 256
#define XIP_BASE 0x10000000

// mock flash storage (flash simulation)
#define MOCK_FLASH_SIZE (256 * 1024) // 256KB
extern uint8_t mock_flash_storage[MOCK_FLASH_SIZE];

// mock flash functions
static inline void flash_range_erase(uint32_t offset, size_t count) {
  if (offset + count <= MOCK_FLASH_SIZE) {
    memset(&mock_flash_storage[offset], 0xFF, count);
  }
}

static inline void flash_range_program(uint32_t offset, const uint8_t *data,
                                       size_t count) {
  if (offset + count <= MOCK_FLASH_SIZE) {
    memcpy(&mock_flash_storage[offset], data, count);
  }
}

// sync functions
static inline uint32_t save_and_disable_interrupts(void) { return 0; }
static inline void restore_interrupts(uint32_t flags) { (void)flags; }

#endif // MOCK_FLASH_H
