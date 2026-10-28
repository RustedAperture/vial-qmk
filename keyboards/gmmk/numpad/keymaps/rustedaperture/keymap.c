/* Copyright 2021 Glorious, LLC <salman@pcgamingrace.com>
 * Modified 2022-2026 by rustedaperture for vial-qmk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "analog.h"
#include "qmk_midi.h"

/* Tap dances TD(0)-TD(6) are Vial tap dances; set them up in Vial (or load the layout file). */

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

//      NUM      /       *       -
//      7        8       9       +
//      4        5       6       ENCODER
//      1        2       3       RET
//      0                        .

  [0] = LAYOUT(
    MO(1),     KC_PSLS,   KC_PAST,   KC_PMNS,
    KC_P7,     KC_P8,     KC_P9,     KC_PPLS,
    KC_P4,     KC_P5,     KC_P6,     KC_SLEP,
    KC_P1,     KC_P2,     KC_P3,     KC_PENT,
    KC_P0,                           KC_PDOT
  ),
  [1] = LAYOUT(
    _______,   TO(2),     TO(3),     KC_F17,
    KC_NO,     TD(4),     KC_F16,    KC_F18,
    KC_MPRV,   KC_MPLY,   KC_MNXT,   KC_CALC,
    QK_BOOT,   KC_NO,     KC_NO,     KC_NO,
    RM_TOGG,                         KC_NO
  ),
  [2] = LAYOUT(
    KC_ESC,    KC_MPLY,   TD(6),     TD(0),
    KC_P,      KC_G,      KC_B,      KC_LSFT,
    KC_Z,      KC_X,      KC_LALT,   KC_NO,
    KC_Y,      MS_BTN5,   KC_PGUP,   KC_SPC,
    KC_LCTL,                         KC_PGDN
  ),
  [3] = LAYOUT(
    KC_ESC,    KC_MPLY,   TD(6),     TD(1),
    KC_NO,     KC_W,      KC_NO,     KC_LSFT,
    KC_Z,      TD(3),     TD(2),     KC_NO,
    KC_Y,      MS_BTN5,   KC_NO,     KC_SPC,
    KC_LCTL,                         TD(5)
  ),
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [2] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [3] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
};
// clang-format on

// Potentiometer Slider, MIDI Control

uint8_t divisor = 0;

void slider(void) {
    if (divisor++) { /* only run the slider function 1/256 times it's called */
        return;
    }
    midi_send_cc(&midi_device, 2, 0x3E, 0x7F + (analogReadPin(SLIDER_PIN) >> 3));
}

void housekeeping_task_user(void) {
    slider();
}
