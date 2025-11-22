#ifndef CDC_DISPATCHER_H
#define CDC_DISPATCHER_H

#include <stdint.h>

/**
 * @brief Initializing the CDC protocol dispatcher.
 */
void cdc_protocol_init(void);

/**
 * @brief Main loop task for the CDC protocol.
 * @note: Should be called in main() loop. Reads data from USB and assembles
 * lines.
 */
void cdc_protocol_task(void);

/**
 * @brief Processes the entire command line.
 * Recognizes the command and delegates it to the appropriate handler.
 * @param cmd Full command line.
 */
void process_command(const char *cmd);

/**
 * @brief Receives the macro script via CDC.
 * @param layer Layer number.
 * @param button Button number.
 * @param platform Target platform.
 * @param script_size Size of the script in bytes.
 */
void cdc_receive_script(uint8_t layer, uint8_t button, uint8_t platform,
                        uint16_t script_size);
/**
 * @brief Receives the key sequence via CDC.
 * @param layer Layer number.
 * @param button Button number.
 * @param count Number of steps in the sequence.
 * @param steps Pointer to the array of steps (keycodes and
 * modifiers).
 */
void cdc_receive_sequence(uint8_t layer, uint8_t button, uint8_t count,
                          uint8_t *steps);

#endif // CDC_DISPATCHER_H
