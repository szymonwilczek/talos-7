#ifndef MOCK_HARDWARE_H
#define MOCK_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

// ==================== MOCK HARDWARE ====================
// Ten moduł symuluje hardware bez fizycznych połączeń

/**
 * @brief Inicjalizacja mock hardware (GPIO, LED, OLED)
 */
void mock_hardware_init(void);

/**
 * @brief Symulacja odczytu przycisku
 * @param button_index Indeks przycisku (0-6)
 * @return true jeśli "wciśnięty" (zawsze false w mock)
 */
bool mock_button_read(uint8_t button_index);

/**
 * @brief Symulacja ustawienia LED
 * @param led_index Indeks LED (0-6)
 * @param state true = włączony, false = wyłączony
 */
void mock_led_set(uint8_t led_index, bool state);

/**
 * @brief Symulacja inicjalizacji OLED
 */
void mock_oled_init(void);

/**
 * @brief Symulacja wyświetlenia tekstu na OLED
 * @param line Numer linii (0-3)
 * @param text Tekst do wyświetlenia
 */
void mock_oled_display(uint8_t line, const char* text);

/**
 * @brief Okresowa aktualizacja mock hardware
 * Wypisuje status do konsoli CDC
 */
void mock_hardware_update(void);

#endif // MOCK_HARDWARE_H
