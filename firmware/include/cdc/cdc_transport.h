
#ifndef CDC_TRANSPORT_H
#define CDC_TRANSPORT_H

#include <stdbool.h>

#define CDC_MAX_COMMAND_LEN 256 ///< Maximum length of a CDC command
#define CDC_FIELD_SEPARATOR '|' ///< Separator for fields in the command
#define CDC_LINE_ENDING '\n'    ///< End of command character

/**
 * @brief Sends a response string via CDC.
 * @note Usage: cdc_send_response("OK|param1|param2");
 * @param response - response string to send.
 */
void cdc_send_response(const char *response);

/**
 * @brief Sends a formatted response string via CDC.
 * @note Usage: cdc_send_response_fmt("VALUE|%d|%s", int_value, str_value);
 * @param format - format string (like in printf).
 * @param ... - arguments to format.
 */
void cdc_send_response_fmt(const char *format, ...);

/**
 * @brief Initializes the CDC protocol handler.
 */
void cdc_protocol_init(void);

/**
 * @brief CDC protocol task handler.
 * @note: It should be called regularly in the main loop of the program.
 */
void cdc_protocol_task(void);

/**
 * @brief Logs a message via CDC.
 * @note Usage: cdc_log("Log message: %d", value);
 * @param format - format string (like in printf).
 * @param ... - arguments to format.
 */
void cdc_log(const char *format, ...);

/**
 * @brief Sets binary mode for CDC protocol.
 * @note When enabled, cdc_protocol_task() will not process incoming data.
 * @param enabled - true to enable binary mode, false to disable.
 */
void cdc_set_binary_mode(bool enabled);

/**
 * @brief Checks if CDC protocol is in binary mode.
 * @return true if binary mode is enabled, false otherwise.
 */
bool cdc_is_binary_mode(void);

#endif // CDC_TRANSPORT_H
