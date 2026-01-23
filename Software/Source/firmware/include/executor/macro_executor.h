#ifndef MACRO_EXECUTOR_H
#define MACRO_EXECUTOR_H

#include <stdint.h>

/**
 * @brief Executes the macro assigned to the specified layer and button.
 * @param layer Layer number where the macro is defined.
 * @param button Button index that triggers the macro.
 */
void execute_macro(uint8_t layer, uint8_t button);

#endif
