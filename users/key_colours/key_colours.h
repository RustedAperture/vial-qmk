// QMK glue for per-key, per-layer RGB colours (see key_colours_core.h for the protocol).
// key_colours.c defines raw_hid_receive_kb, eeconfig_init_user_datablock,
// keyboard_post_init_user, housekeeping_task_user and rgb_matrix_indicators_advanced_user,
// so the keymap must not define any of them.
#pragma once
#include "key_colours_core.h"

// Hooks for other modules. The weak defaults live in key_colours.c.
// Called when key_colours does not handle a raw HID packet; return true if handled (reply in place in data).
bool key_colours_raw_hid_user(uint8_t *data, uint8_t length);
// Called at the end of each rgb_matrix_indicators_advanced_user call, after the key colours, with the same range.
void key_colours_render_user(uint8_t led_min, uint8_t led_max);
// kc_to_led(c, rgb_matrix_get_val()): the same gamma, balance and brightness as the key colours.
kc_rgb_t key_colours_to_led(kc_rgb_t c);
