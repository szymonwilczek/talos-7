
#ifndef EXEC_TEXT_H
#define EXEC_TEXT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Sends a Unicode codepoint using platform-specific methods.
 * @param platform Target platform (0-Linux, 1-Win, 2-Mac).
 * @param codepoint Unicode codepoint to send.
 */
void send_unicode(uint8_t platform, uint32_t codepoint);

/**
 * @brief Checks if a string contains only pure ASCII characters.
 * @param str Input string to check.
 * @return true if the string is pure ASCII, false otherwise.
 */
bool is_pure_ascii(const char *str);

/**
 * @brief Types text quickly assuming it is pure ASCII.
 * @param text ASCII text to type.
 */
void type_text_turbo_ascii(const char *text);

/**
 * @brief Types text content considering platform-specific methods.
 * @param text Text to type.
 * @param platform Target platform (0-Linux, 1-Win, 2-Mac).
 */
void type_text_content(const char *text, uint8_t platform);

/**
 * @brief Automatically types text, optimizing for pure ASCII when possible.
 * @param text Text to type.
 */
void exec_type_text_auto(const char *text);

#endif // EXEC_TEXT_H
