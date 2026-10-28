// QMK glue for per-key, per-layer RGB colours (see key_colours_core.h for the protocol).
// key_colours.c defines raw_hid_receive_kb, eeconfig_init_user_datablock,
// keyboard_post_init_user, housekeeping_task_user and rgb_matrix_indicators_advanced_user,
// so the keymap must not define any of them.
#pragma once
#include "key_colours_core.h"
