# slider_targets

A list of MIDI targets for a slider (potentiometer), stored on the keyboard and edited from the **Slider**
tab in [RustedAperture/vial-gui `key-colours`](https://github.com/RustedAperture/vial-gui/tree/key-colours).

- Up to 8 targets, each with a MIDI control number (0-127), a colour and a name (up to 12 ASCII characters).
- The slider sends MIDI CC on channel index 2 to the current target's control. It is read every 10 ms;
  small jitter is ignored, and the bottom and top few counts always resolve to 0 and 127.
- The `SLIDER_NEXT` key (`QK_KB_0`) moves to the next target, wrapping round, and flashes the side LEDs in
  that target's colour for 1 s. Pressing it again during the flash restarts it with the new colour.
- The current target is remembered across power cycles.
- Default list: one target, control 62, named "Slider", white.

Like the rest of Vial's saved settings, the targets are reset the first time a newly built firmware boots.
The app saves them in `.vil` layout files, so restoring a layout restores them too.

## Configuration
Define these in the keymap's `config.h` (the core uses `#ifndef` defaults):
- `ST_ADC_MIN` (default 0) and `ST_ADC_MAX` (default 1023): raw ADC counts at the slider's bottom and top.
  `st_scale` maps this range linearly to 0..127 (with rounding); readings within `ST_HYSTERESIS` of either end,
  or beyond it, send exactly 0 or 127.
- `ST_HYSTERESIS` (default 6): raw counts a reading must move before it is considered.

To calibrate, record the MIDI values while moving the slider fully down and up a few times. If they never
reach 0 or 127, find the raw readings at each end. Set `ST_ADC_MIN` a few counts above the highest bottom
reading and `ST_ADC_MAX` a few counts below the lowest top reading, so both ends reliably reach the end
zones. The numpad uses 16 and 940 (bottom read about 7-12, top about 947-954).

## Files
- `slider_targets_core.{h,c}`: storage, raw HID protocol, next-target and flash timing, slider scaling with
  hysteresis. Plain C with no QMK dependencies, unit-tested on the host.
- `slider_targets.{h,c}`: QMK glue (keyboard EEPROM datablock, `eeconfig_init_kb_datablock`, slider
  reading and MIDI, flash rendering).
- `test/`: host tests for the core, built with the default range and with the numpad's calibrated range.

## Using it in a keymap
Needs MIDI (`MIDI_ADVANCED`), `SLIDER_PIN`, VIA/Vial, and `users/key_colours` (for raw HID and rendering).
The module takes no QMK hooks except `eeconfig_init_kb_datablock`, so the keyboard must not use its
EEPROM datablock for anything else. Call its functions from your keymap:

    #include "key_colours.h"
    #include "slider_targets.h"

    void housekeeping_task_user(void) { slider_targets_task(); }
    bool process_record_user(uint16_t keycode, keyrecord_t *record) {
        return slider_targets_process_record(keycode, record);
    }
    bool key_colours_raw_hid_user(uint8_t *data, uint8_t length) { return slider_targets_raw_hid(data, length); }
    void key_colours_render_user(uint8_t led_min, uint8_t led_max) { slider_targets_render(led_min, led_max); }

The targets are loaded on the first call to any of these, so no `keyboard_post_init_user` is needed.

`rules.mk`:

    VPATH += $(TOP_DIR)/users/slider_targets
    SRC += slider_targets_core.c slider_targets.c

`config.h`:

    #define EECONFIG_KB_DATA_SIZE 130            /* sizeof(st_store_t) */
    #define EECONFIG_KB_DATA_VERSION (0x53540000 | 130)
    #define SLIDER_TARGETS_FLASH_LEDS { 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 }

A static assert catches a wrong size at compile time.

Put `QK_KB_0` on a key, and add it to `vial.json` so Vial shows its name:

    "customKeycodes": [
      {"name": "SLIDER_NEXT", "title": "Next slider target", "shortName": "Slider\nNext"}
    ]

### Hooks
Declared in `slider_targets.h`:

- `void slider_targets_task(void)`: call from `housekeeping_task_user`. Reads the slider and sends MIDI.
- `bool slider_targets_process_record(uint16_t keycode, keyrecord_t *record)`: returns false if it
  handled `SLIDER_NEXT` (on press; the release is swallowed too).
- `bool slider_targets_raw_hid(uint8_t *data, uint8_t length)`: returns true if it handled the packet
  (reply in place in `data`).
- `void slider_targets_render(uint8_t led_min, uint8_t led_max)`: paints the flash LEDs in that range
  while the flash is showing, with `key_colours_to_led` (same gamma, balance and brightness as key colours).

## Raw HID protocol
Command byte `0x56`, 32-byte packets, sub-command in byte 1. Errors set byte 1 to `0xFF` and change nothing.
Firmware without this module replies with byte 0 set to `0xFF`.

| Sub | Name | Request | Reply |
|---|---|---|---|
| 0x01 | GET_INFO | | `[2]` version 1, `[3]` max targets 8, `[4]` count, `[5]` current index, `[6]` name max 12 |
| 0x02 | GET | `[2]` index (< count) | `[2]` index, `[3]` control, `[4..6]` r g b, `[7..18]` name (zero-padded) |
| 0x03 | SET | `[2]` index (< 8), `[3]` control (0-127), `[4..6]` r g b, `[7..18]` name | request echoed |
| 0x04 | SET_COUNT | `[2]` count 1-8 | request echoed; current index reset to 0 if past the end |

To reorder, add or remove targets, write them with SET (the index may be past the current count), then
SET_COUNT.

## Tests

    make -C users/slider_targets/test
