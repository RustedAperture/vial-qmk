/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define VIAL_KEYBOARD_UID {0x5B, 0x0D, 0xC1, 0xFA, 0xB0, 0x63, 0x9A, 0xB4}

/* Unlock combo: NUM + . */
#define VIAL_UNLOCK_COMBO_ROWS { 0, 4 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 3 }

#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_TYPING_HEATMAP

/* Per-key, per-layer colours (users/key_colours) */
#define KEY_COLOURS_LAYERS 4
#define KEY_COLOURS_LEDS 31
#define EECONFIG_USER_DATA_SIZE 389          /* 1 + 4*4 + 4*31*3 */
/* Format version follows the counts; bump the high part (0x4B43xxxx) when kc_store_t changes */
#define EECONFIG_USER_DATA_VERSION (0x4B430000 | (KEY_COLOURS_LAYERS << 8) | KEY_COLOURS_LEDS)

/* Slider MIDI targets (users/slider_targets) */
#define EECONFIG_KB_DATA_SIZE 130            /* sizeof(st_store_t) */
/* Bump the high part (0x5354xxxx) when st_store_t changes */
#define EECONFIG_KB_DATA_VERSION (0x53540000 | 130)
#define SLIDER_TARGETS_FLASH_LEDS { 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 }
