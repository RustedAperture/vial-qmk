/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define VIAL_KEYBOARD_UID {0x8C, 0x7B, 0x30, 0x96, 0xAD, 0x72, 0xCD, 0xCB}

#define VIAL_UNLOCK_COMBO_ROWS { 0, 5, 3, 8 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 0, 0, 0 }

#define SPLIT_USB_DETECT

/* Send WPM, key activity, mods and caps lock to the non-USB half for its Neko OLED */
#define SPLIT_WPM_ENABLE
#define SPLIT_ACTIVITY_ENABLE
#define SPLIT_MODS_ENABLE
#define SPLIT_LED_STATE_ENABLE

/* Per-key, per-layer colours (users/key_colours) */
#define KEY_COLOURS_LAYERS 4
#define KEY_COLOURS_LEDS 58
#define EECONFIG_USER_DATA_SIZE 729          /* 1 + 4*8 + 4*58*3 */
#define EECONFIG_USER_DATA_VERSION 0x4B430001 /* bump when the store format changes */
#define SPLIT_TRANSACTION_IDS_USER KEY_COLOURS_SYNC
