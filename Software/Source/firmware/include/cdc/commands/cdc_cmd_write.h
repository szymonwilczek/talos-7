#ifndef CDC_CMD_WRITE_H
#define CDC_CMD_WRITE_H

/**
 * @brief Handles the SET_MACRO|... command.
 * Parses macro settings.
 * @note Usage: SET_MACRO|layer|button|type|value|string|name|emoji_index
 * @param args String with parameters separated by '|'.
 */
void cmd_handle_set_macro(char *args);

/**
 * @brief Handles the SET_LAYER_NAME|... command.
 * @note Usage: SET_LAYER_NAME|layer|name|emoji_index
 * @param args String with parameters.
 */
void cmd_handle_set_layer_name(char *args);

/**
 * @brief Handles the SET_MACRO_SEQ|... command.
 * Parses and stores a macro key sequence.
 * @note Usage: SET_MACRO_SEQ|layer|button|count|step1|step2|...|stepN
 * @param args String with parameters separated by '|'.
 */
void cmd_handle_set_macro_seq(char *args);

/**
 * @brief Handles the SET_MACRO_SCRIPT|... command.
 * Prepares the buffer and starts the script reception procedure. Then the raw
 * bytes of the script are sent.
 * @note Usage: SET_MACRO_SCRIPT|layer|button|platform|size
 * @param args String of characters with a script header.
 */
void cmd_handle_set_macro_script(char *args);

#endif // CDC_CMD_WRITE_H
