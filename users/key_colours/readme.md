# key_colours

Per-key, per-layer RGB colours, stored on the keyboard and edited live from the **Key colours** tab in
[RustedAperture/vial-gui `key-colours`](https://github.com/RustedAperture/vial-gui/tree/key-colours).

- Stores a colour (or "none") for every RGB Matrix LED on every layer, in QMK's user EEPROM datablock.
- LEDs with no colour show the normal lighting effect underneath.
- Colours follow the highest active layer. While the app's Key colours tab is open, the keyboard previews
  the layer being painted; the preview drops out 3 s after the app stops refreshing it.
- Painted colours obey the RGB on/off switch and brightness.
- Split keyboards: the USB half sends the colour table to the other half over a split RPC, and resends it
  all after a reconnect.
- Colours are gamma-corrected (2.2) and colour-balanced per board before reaching the LEDs, so they
  match what the app shows.

Like the rest of Vial's saved settings, colours are wiped the first time a newly built firmware boots.
The app saves them in `.vil` layout files, so restoring a layout restores colours too.

## Files
- `key_colours_core.{h,c}`: storage, raw HID protocol, preview, split sync bookkeeping, colour
  conversion. Plain C with no QMK dependencies, unit-tested on the host.
- `key_colours.{h,c}`: QMK glue (`raw_hid_receive_kb`, user EEPROM datablock, split RPC,
  `rgb_matrix_indicators_advanced_user`).
- `test/`: host tests for the core.

## Using it in a keymap
Needs VIA/Vial and VialRGB (`VIALRGB_ENABLE = yes`). The keymap must not define the hooks listed above
(`housekeeping_task_user` is only taken on split keyboards). Works on split and non-split keyboards.

`rules.mk`:

    VPATH += $(TOP_DIR)/users/key_colours
    SRC += key_colours_core.c key_colours.c

`config.h` (example for 4 layers and 58 LEDs):

    #define KEY_COLOURS_LAYERS 4                 /* must equal DYNAMIC_KEYMAP_LAYER_COUNT */
    #define KEY_COLOURS_LEDS 58                  /* must equal RGB_MATRIX_LED_COUNT */
    #define EECONFIG_USER_DATA_SIZE 729          /* 1 + layers * ceil(leds / 8) + layers * leds * 3 */
    #define EECONFIG_USER_DATA_VERSION (0x4B430000 | (KEY_COLOURS_LAYERS << 8) | KEY_COLOURS_LEDS)
    #define SPLIT_TRANSACTION_IDS_USER KEY_COLOURS_SYNC   /* split keyboards only */

Static asserts catch a wrong size or count at compile time.

### LED colour balance
Many WS2812 LEDs have brighter green and blue than red, so whites look blue and yellows look lime.
Set per-channel balance (0-255, default 255) in `config.h`:

    #define KEY_COLOURS_BALANCE_R 255
    #define KEY_COLOURS_BALANCE_G 140
    #define KEY_COLOURS_BALANCE_B 95

Calibrate by eye: paint a key yellow and lower green until it stops looking lime, then paint it white and
lower blue until it looks neutral. The values above are for the Sofle Pico's LEDs.

## Raw HID protocol
Command byte `0x55`, 32-byte packets, sub-command in byte 1. Errors set byte 1 to `0xFF`.
Each colour entry is 4 bytes: `[set, r, g, b]`, at most 6 per packet.

| Sub | Name | Request | Reply |
|---|---|---|---|
| 0x01 | GET_INFO | | `[2]` version 1, `[3]` layers, `[4..5]` LED count LE16, `[6]` initialised |
| 0x02 | GET | `[2]` layer, `[3]` start LED, `[4]` count 1-6 | request echoed, entries from `[5]` |
| 0x03 | SET | `[2]` layer, `[3]` start, `[4]` count 1-6, entries from `[5]` | request echoed |
| 0x04 | PREVIEW | `[2]` layer, or `0xFF` for off | request echoed |

## Tests

    make -C users/key_colours/test

## Building the Sofle Pico keymap

    make sofle_pico:rustedaperture
    # or with Docker:
    docker run --rm -v "$PWD":/qmk_firmware -w /qmk_firmware ghcr.io/qmk/qmk_cli make sofle_pico:rustedaperture
