
#ifndef CDC_TRANSPORT_H
#define CDC_TRANSPORT_H

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

#endif // CDC_TRANSPORT_H
