#ifndef MACRO_CONFIG_H
#define MACRO_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

// ==================== CONFIGURATION CONSTANTS ====================
#define MAX_LAYERS 4
#define NUM_BUTTONS 7
#define MACRO_STRING_LEN 32
#define MAX_NAME_LEN 16
#define MAX_EMOJI_LEN 8
#define MAX_SCRIPT_SIZE 2048
#define FLASH_SECTOR_SIZE_CALC                                                 \
  ((sizeof(config_data_t) + 4095) & ~4095) // zaokraglenie do wieloktronosci 4KB

// ==================== SEKWENCJE KLAWISZY ====================
typedef struct {
  uint8_t keycode;   // HID keycode
  uint8_t modifiers; // modyfikatory (bitmask)
} key_step_t;

#define MAX_SEQUENCE_STEPS 5
#define MODIFIER_LEFT_CTRL (1 << 0)   // 0x01
#define MODIFIER_LEFT_SHIFT (1 << 1)  // 0x02
#define MODIFIER_LEFT_ALT (1 << 2)    // 0x04
#define MODIFIER_LEFT_GUI (1 << 3)    // 0x08 (Windows/Super/Cmd)
#define MODIFIER_RIGHT_CTRL (1 << 4)  // 0x10
#define MODIFIER_RIGHT_SHIFT (1 << 5) // 0x20
#define MODIFIER_RIGHT_ALT (1 << 6)   // 0x40
#define MODIFIER_RIGHT_GUI (1 << 7)   // 0x80

// ==================== TYPY MAKR ====================
typedef enum {
  MACRO_TYPE_KEY_PRESS = 0,    // pojedynczy klawisz (HID keycode)
  MACRO_TYPE_TEXT_STRING = 1,  // tekst do wpisania
  MACRO_TYPE_LAYER_TOGGLE = 2, // przelaczanie warstw
  MACRO_TYPE_SCRIPT = 3,       // skrypt do wykonania
  MACRO_TYPE_KEY_SEQUENCE = 4  // sekwencja klawiszy
} macro_type_t;

// ==================== STRUKTURA MAKRA ====================
typedef struct {
  macro_type_t type;
  uint16_t value;                          // keycode lub target layer
  char macro_string[MACRO_STRING_LEN];     // tekst makra
  char name[MAX_NAME_LEN];                 // nazwa makra
  uint8_t emoji_index;                     // indeks emoji
  char script[MAX_SCRIPT_SIZE];            // skrypt makra
  uint8_t script_platform;                 // platforma skryptu
  key_step_t sequence[MAX_SEQUENCE_STEPS]; // sekwencja klawiszy
  uint8_t sequence_length;                 // dlugosc sekwencji
} macro_entry_t;

// ==================== GLOBALNA KONFIGURACJA ====================
typedef struct {
  char layer_names[MAX_LAYERS][MAX_NAME_LEN];    // nazwy warstw
  uint8_t layer_emojis[MAX_LAYERS];              // emoji warstw
  macro_entry_t macros[MAX_LAYERS][NUM_BUTTONS]; // wszystkie makra
  uint32_t crc32;                                // checksum
} config_data_t;

// ==================== FLASH STORAGE ====================
#define FLASH_TARGET_OFFSET (1024 * 1024) // 1MB offset
#define FLASH_SECTOR_SIZE 4096            // 4KB sector

// ==================== FUNKCJE PUBLICZNE ====================

void config_init(void);
config_data_t *config_get(void);
bool config_save(void);
void config_set_factory_defaults(void);
uint32_t config_calculate_crc(const config_data_t *config);
uint8_t config_get_current_layer(void);
void config_cycle_layer(void);

#endif // MACRO_CONFIG_H
