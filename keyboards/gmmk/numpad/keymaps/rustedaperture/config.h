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
/* Flash every LED (keys 0-16 and both side strips 17-30) in the new target's colour */
#define SLIDER_TARGETS_FLASH_LEDS { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, \
                                    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 }

/* Slider ADC range, calibrated from recorded MIDI on 2026-10-28 (bottom read ~7-12, top ~947-954).
 * Chosen with margin so both ends reliably reach 0 and 127. */
#define ST_ADC_MIN 16
#define ST_ADC_MAX 940
