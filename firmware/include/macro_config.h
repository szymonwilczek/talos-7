#ifndef MACRO_CONFIG_H
#define MACRO_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "pin_definitions.h"

// ==================== CONFIGURATION CONSTANTS ====================
#define MAX_NAME_LEN        16
#define MACRO_STRING_LEN    32

// ==================== MACRO TYPES ====================
typedef enum {
    MACRO_TYPE_KEY_PRESS    = 0,  // pojedynczy klawisz HID
    MACRO_TYPE_MACRO_STRING = 1,  // ciag znakow do wpisania
    MACRO_TYPE_LAYER_TOGGLE = 2   // przelaczenie warstwy
} macro_type_t;

// ==================== MACRO ENTRY STRUCTURE ====================
typedef struct {
    uint8_t type;                           // macro_type_t
    uint16_t value;                         // HID keycode (type 0, 2) lub długość (type 1)
    char macro_string[MACRO_STRING_LEN];    // string dla MACRO_STRING
    char name[MAX_NAME_LEN];                // nazwa wyswietlana
} macro_entry_t;

// ==================== GLOBAL CONFIGURATION ====================
typedef struct {
    macro_entry_t macros[MAX_LAYERS][NUM_BUTTONS];
    char layer_names[MAX_LAYERS][MAX_NAME_LEN];
    uint32_t crc32;  // suma kontrolna konfiguracji
} config_data_t;

// ==================== FLASH MEMORY LAYOUT ====================
// offset od konca flash (256KB - 4KB = 252KB)
#define FLASH_TARGET_OFFSET (252 * 1024)

// ==================== FUNKCJE PUBLICZNE ====================

/**
 * @brief inicjalizacja systemu konfiguracji
 * laduje konfiguracje z flash lub ustawia domyslna
 */
void config_init(void);

/**
 * @brief zapisuje aktualna konfiguracje do flash
 * @return true jesli sukces, false jesli błąd
 */
bool config_save(void);

/**
 * @brief pobiera wskaznik do aktualnej konfiguracji
 * @return wskaznik do struktury config_data_t
 */
config_data_t* config_get(void);

/**
 * @brief ustawia konfiguracje fabryczna (Layer 0: F1-F7)
 */
void config_set_factory_defaults(void);

/**
 * @brief oblicza CRC32 dla danych konfiguracji
 * @param config wskaznik do konfiguracji
 * @return wartosc CRC32
 */
uint32_t config_calculate_crc(const config_data_t* config);

#endif // MACRO_CONFIG_H
