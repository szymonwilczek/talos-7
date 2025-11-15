#ifndef CDC_PROTOCOL_H
#define CDC_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// ==================== PROTOKÓŁ CDC ====================
// maksymalna dlugosc komendy
#define CDC_MAX_COMMAND_LEN     256

// separator pol w komendzie
#define CDC_FIELD_SEPARATOR     '|'

// zakonczenie komendy
#define CDC_LINE_ENDING         '\n'

// ==================== KOMENDY ====================
// GET_CONF - pobranie calej konfiguracji
// SET_MACRO|layer|button|type|value|string|name - ustawienie makra
// SET_LAYER_NAME|layer|name - ustawienie nazwy warstwy
// SAVE_FLASH - wymuszenie zapisu do flash

// ==================== FUNKCJE PUBLICZNE ====================

/**
 * @brief inicjalizacja modulu protokolu CDC
 */
void cdc_protocol_init(void);

/**
 * @brief glowna petla obslugi CDC
 * odbiera znaki, buforuje i przetwarza komendy
 */
void cdc_protocol_task(void);

/**
 * @brief wysyla odpowiedz przez CDC
 * @param response string do wyslania (automatycznie dodaje \r\n)
 */
void cdc_send_response(const char* response);

/**
 * @brief wysyla sformatowana odpowiedz
 * @param format format string
 * @param ... argumenty
 */
void cdc_send_response_fmt(const char* format, ...);

#endif // CDC_PROTOCOL_H
