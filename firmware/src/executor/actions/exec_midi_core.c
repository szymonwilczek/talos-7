#include "executor/actions/exec_midi_core.h"

#include "cdc/cdc_transport.h"
#include "pico/stdlib.h"
#include "tusb.h"

void exec_midi_note(uint8_t note, uint8_t velocity, uint8_t channel) {
  // channel conversion from 1-16 to 0-15 (midi protocol)
  // channel = 0 (default), assign to 0 (channel 1)
  uint8_t midi_channel = (channel > 0) ? channel - 1 : 0;

  // safety clamps
  if (note > 127)
    note = 127;
  if (velocity > 127)
    velocity = 127;
  if (midi_channel > 15)
    midi_channel = 0; // fallback to ch 1

  uint8_t msg[3];

  // --- Note ON ---
  msg[0] = 0x90 | midi_channel; // 0x90 = Note On status for channel
  msg[1] = note;
  msg[2] = velocity;

  if (tud_midi_mounted()) {
    tud_midi_stream_write(0, msg, 3);
    cdc_log("[MIDI] Note ON: %d Vel: %d Ch: %d\n", note, velocity,
            midi_channel + 1);
  }

  sleep_ms(100);

  // --- Note OFF ---
  msg[0] = 0x80 | midi_channel; // 0x80 = Note Off status
  msg[2] = 0;                   // velocity 0

  if (tud_midi_mounted()) {
    tud_midi_stream_write(0, msg, 3);
    cdc_log("[MIDI] Note OFF\n");
  }
}
