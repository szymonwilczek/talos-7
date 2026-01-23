#ifndef EXEC_MIDI_CORE_H
#define EXEC_MIDI_CORE_H

#include <stdint.h>

/**
 * @brief Sends a MIDI Note On followed by Note Off.
 * @param note MIDI Note number (0-127).
 * @param velocity Note velocity (0-127).
 * @param channel MIDI Channel (1-16).
 */
void exec_midi_note(uint8_t note, uint8_t velocity, uint8_t channel);

/**
 * @brief Sends a MIDI Control Change message.
 * @param controller Controller Number (0-119).
 * @param value Controller Value (0-127).
 * @param channel MIDI Channel (1-16).
 */
void exec_midi_cc(uint8_t controller, uint8_t value, uint8_t channel);

#endif // EXEC_MIDI_CORE_H
