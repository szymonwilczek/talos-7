#ifndef COMPANION_RENDERER_H
#define COMPANION_RENDERER_H

#include "companion/companion_state.h"
#include <stdint.h>

/**
 * @brief Initializes renderer caches (contrast, power, pixel-shift state).
 */
void companion_renderer_init(void);

/**
 * @brief Renders the full 128x64 Companion Mode frame and updates OLED power/contrast.
 * @param state Current companion state machine snapshot.
 * @param now_ms Current monotonic millisecond timestamp.
 */
void companion_renderer_render(const companion_state_t *state, uint32_t now_ms);

#endif // COMPANION_RENDERER_H
