#ifndef MACRO_CONFIG_H
#define MACRO_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ==================== CONFIGURATION CONSTANTS ====================
#define MAX_LAYERS              4
#define NUM_BUTTONS             7
#define MACRO_STRING_LEN        32
#define MAX_NAME_LEN            16
#define MAX_EMOJI_LEN           8      // NOWE: UTF-8 emoji

// ==================== TYPY MAKR ====================
typedef enum {
    MACRO_TYPE_KEY_PRESS = 0,    // pojedynczy klawisz (HID keycode)
    MACRO_TYPE_TEXT_STRING = 1,  // tekst do wpisania
    MACRO_TYPE_LAYER_TOGGLE = 2  // przelaczanie warstw
} macro_type_t;

// ==================== STRUKTURA MAKRA ====================
typedef struct {
    macro_type_t type;
    uint16_t value;                          // keycode lub target layer
    char macro_string[MACRO_STRING_LEN];     // tekst makra
    char name[MAX_NAME_LEN];                 // nazwa makra
    char emoji[MAX_EMOJI_LEN];               // NOWE: emoji przycisku (UTF-8)
} macro_entry_t;

// ==================== GLOBALNA KONFIGURACJA ====================
typedef struct {
    char layer_names[MAX_LAYERS][MAX_NAME_LEN];     // nazwy warstw
    char layer_emojis[MAX_LAYERS][MAX_EMOJI_LEN];   // NOWE: emoji warstw
    macro_entry_t macros[MAX_LAYERS][NUM_BUTTONS];  // wszystkie makra
    uint32_t crc32;                                  // checksum (już istnieje)
} config_data_t;

// ==================== FLASH STORAGE ====================
#define FLASH_TARGET_OFFSET     (1024 * 1024)  // 1MB offset
#define FLASH_SECTOR_SIZE       4096            // 4KB sector

// ==================== FUNKCJE PUBLICZNE ====================

void config_init(void);
config_data_t* config_get(void);
bool config_save(void);
void config_set_factory_defaults(void);
uint32_t config_calculate_crc(const config_data_t* config);

#endif // MACRO_CONFIG_H
