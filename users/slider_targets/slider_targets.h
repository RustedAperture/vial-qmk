// QMK glue for the numpad slider's MIDI targets (see slider_targets_core.h for the protocol).
// Defines eeconfig_init_kb_datablock; the keymap calls the functions below from its own hooks.
#pragma once
#include "quantum.h"
#include "slider_targets_core.h"

#define SLIDER_NEXT QK_KB_0

// Call from housekeeping_task_user. Loads the targets on first call, reads the slider every 10 ms.
void slider_targets_task(void);
// Call from process_record_user; returns false if it handled SLIDER_NEXT.
bool slider_targets_process_record(uint16_t keycode, keyrecord_t *record);
// Call from key_colours_raw_hid_user (or raw_hid_receive_kb); returns true if handled (reply in place).
bool slider_targets_raw_hid(uint8_t *data, uint8_t length);
// Call from key_colours_render_user; paints the LEDs in the flash mask while the flash is showing.
void slider_targets_render(uint8_t led_min, uint8_t led_max);
