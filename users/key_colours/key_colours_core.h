// Pure-C core for per-key, per-layer RGB colours. No QMK dependencies.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef KEY_COLOURS_LAYERS
#error "define KEY_COLOURS_LAYERS before including key_colours_core.h"
#endif
#ifndef KEY_COLOURS_LEDS
#error "define KEY_COLOURS_LEDS before including key_colours_core.h"
#endif
_Static_assert(KEY_COLOURS_LEDS <= 192, "dirty mask is 32 chunks of 6 LEDs");

/* Per-channel LED balance (0-255) applied after gamma, so a white key looks white.
   Calibrate per board by eye; 255 means unchanged. */
#ifndef KEY_COLOURS_BALANCE_R
#define KEY_COLOURS_BALANCE_R 255
#endif
#ifndef KEY_COLOURS_BALANCE_G
#define KEY_COLOURS_BALANCE_G 255
#endif
#ifndef KEY_COLOURS_BALANCE_B
#define KEY_COLOURS_BALANCE_B 255
#endif

#define KEY_COLOURS_CMD 0x55
#define KC_GET_INFO 0x01
#define KC_GET 0x02
#define KC_SET 0x03
#define KC_PREVIEW 0x04
#define KC_ERR 0xFF
#define KC_PROTOCOL_VERSION 1
#define KC_PER_PACKET 6
#define KC_ENTRY_SIZE 4
#define KC_PREVIEW_OFF 0xFF
#define KC_PREVIEW_TIMEOUT_MS 3000

typedef struct __attribute__((packed)) { uint8_t r, g, b; } kc_rgb_t;

typedef struct __attribute__((packed)) {
    uint8_t  initialised;
    uint8_t  set_bits[KEY_COLOURS_LAYERS][(KEY_COLOURS_LEDS + 7) / 8];
    kc_rgb_t rgb[KEY_COLOURS_LAYERS][KEY_COLOURS_LEDS];
} kc_store_t;

typedef struct {
    uint8_t  preview_layer;             // KC_PREVIEW_OFF when off
    uint32_t preview_deadline;
    uint32_t dirty[KEY_COLOURS_LAYERS]; // bit n = LEDs [6n, 6n+6) need syncing
    bool     preview_dirty;
} kc_runtime_t;

typedef struct {
    bool    store_changed;
    uint8_t layer, start, count;
    bool    initialised_changed;
} kc_change_t;

void     kc_store_blank(kc_store_t *s);
void     kc_runtime_init(kc_runtime_t *rt);
bool     kc_get(const kc_store_t *s, uint8_t layer, uint8_t led, kc_rgb_t *out); // true if set
void     kc_put(kc_store_t *s, uint8_t layer, uint8_t led, bool set, kc_rgb_t c);
uint8_t  kc_pack(const kc_store_t *s, uint8_t layer, uint8_t start, uint8_t count, uint8_t *out);
bool     kc_unpack(kc_store_t *s, uint8_t layer, uint8_t start, uint8_t count, const uint8_t *in);
bool     kc_handle_hid(kc_store_t *s, kc_runtime_t *rt, uint8_t *data, uint8_t length, uint32_t now, kc_change_t *change);
void     kc_set_preview(kc_runtime_t *rt, uint8_t layer, uint32_t now);
uint8_t  kc_display_layer(const kc_runtime_t *rt, uint8_t active_layer, uint32_t now);
kc_rgb_t kc_scale(kc_rgb_t c, uint8_t val);
uint8_t  kc_gamma(uint8_t v);                // screen (sRGB-ish, gamma 2.2) value -> linear LED value
kc_rgb_t kc_to_led(kc_rgb_t c, uint8_t val); // gamma, then channel balance, then brightness
void     kc_mark_dirty(kc_runtime_t *rt, uint8_t layer, uint8_t start, uint8_t count);
void     kc_mark_all_dirty(kc_runtime_t *rt);
bool     kc_next_dirty_chunk(kc_runtime_t *rt, uint8_t *layer, uint8_t *start, uint8_t *count);
