// QMK glue: EEPROM persistence, raw HID, split sync and rendering for key colours.
#include QMK_KEYBOARD_H
#include "via.h"
#include "transactions.h"
#include "split_util.h"
#include "key_colours.h"

_Static_assert(sizeof(kc_store_t) == EECONFIG_USER_DATA_SIZE, "EECONFIG_USER_DATA_SIZE must match kc_store_t");
_Static_assert(KEY_COLOURS_LAYERS == DYNAMIC_KEYMAP_LAYER_COUNT, "KEY_COLOURS_LAYERS must match DYNAMIC_KEYMAP_LAYER_COUNT");
_Static_assert(KEY_COLOURS_LEDS == RGB_MATRIX_LED_COUNT, "KEY_COLOURS_LEDS must match RGB_MATRIX_LED_COUNT");

#define KC_SYNC_INTERVAL_MS 10
#define KC_SYNC_CHUNK 0
#define KC_SYNC_PREVIEW 1
#define KC_SYNC_HEADER 4
_Static_assert(KC_SYNC_HEADER + KC_PER_PACKET * KC_ENTRY_SIZE <= RPC_M2S_BUFFER_SIZE, "sync chunk exceeds RPC buffer");

static kc_store_t   kc_store;
static kc_runtime_t kc_rt;

// ---- EEPROM ----

void eeconfig_init_user_datablock(void) {
    kc_store_t blank;
    kc_store_blank(&blank);
    eeconfig_update_user_datablock(&blank, 0, sizeof blank);
}

static void kc_save_change(const kc_change_t *ch) {
    if (ch->initialised_changed) {
        eeconfig_update_user_datablock(&kc_store.initialised, offsetof(kc_store_t, initialised), sizeof kc_store.initialised);
    }
    eeconfig_update_user_datablock(kc_store.set_bits[ch->layer], offsetof(kc_store_t, set_bits) + (uint32_t)ch->layer * sizeof kc_store.set_bits[0], sizeof kc_store.set_bits[0]);
    eeconfig_update_user_datablock(&kc_store.rgb[ch->layer][ch->start], offsetof(kc_store_t, rgb) + ((uint32_t)ch->layer * KEY_COLOURS_LEDS + ch->start) * sizeof(kc_rgb_t), (uint32_t)ch->count * sizeof(kc_rgb_t));
}

// ---- Raw HID ----

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    kc_change_t ch;
    if (!kc_handle_hid(&kc_store, &kc_rt, data, length, timer_read32(), &ch)) {
        data[0] = id_unhandled;
        return;
    }
    if (ch.store_changed) kc_save_change(&ch);
}

// ---- Split sync ----

static void kc_sync_slave_handler(uint8_t in_len, const void *in, uint8_t out_len, void *out) {
    (void)out_len;
    (void)out;
    const uint8_t *p = (const uint8_t *)in;
    if (in_len < KC_SYNC_HEADER) return;
    if (p[0] == KC_SYNC_CHUNK) {
        uint8_t count = p[3];
        if (count > KC_PER_PACKET || in_len < KC_SYNC_HEADER + count * KC_ENTRY_SIZE) return;
        kc_unpack(&kc_store, p[1], p[2], count, &p[KC_SYNC_HEADER]);
    } else if (p[0] == KC_SYNC_PREVIEW) {
        uint16_t remaining = (uint16_t)(p[2] | (p[3] << 8));
        uint32_t now       = timer_read32();
        if (remaining == 0 || p[1] >= KEY_COLOURS_LAYERS) {
            kc_set_preview(&kc_rt, KC_PREVIEW_OFF, now);
        } else {
            kc_set_preview(&kc_rt, p[1], now);
            kc_rt.preview_deadline = now + remaining;
        }
    }
}

static void kc_sync_master(void) {
    static bool     was_connected = false;
    static uint32_t last_sync     = 0;

    bool connected = is_transport_connected();
    if (connected && !was_connected) kc_mark_all_dirty(&kc_rt);
    was_connected = connected;
    if (!connected) return;
    if (timer_elapsed32(last_sync) < KC_SYNC_INTERVAL_MS) return;

    uint8_t buf[KC_SYNC_HEADER + KC_PER_PACKET * KC_ENTRY_SIZE];
    if (kc_rt.preview_dirty) {
        uint32_t now       = timer_read32();
        uint8_t  layer     = kc_rt.preview_layer;
        uint16_t remaining = 0;
        if (layer != KC_PREVIEW_OFF && (int32_t)(kc_rt.preview_deadline - now) > 0) {
            uint32_t r = kc_rt.preview_deadline - now;
            remaining  = r > 0xFFFF ? 0xFFFF : (uint16_t)r;
        } else {
            layer = KC_PREVIEW_OFF;
        }
        buf[0]             = KC_SYNC_PREVIEW;
        buf[1]             = layer;
        buf[2]             = (uint8_t)(remaining & 0xFF);
        buf[3]             = (uint8_t)(remaining >> 8);
        kc_rt.preview_dirty = false;
        last_sync          = timer_read32();
        if (!transaction_rpc_send(KEY_COLOURS_SYNC, KC_SYNC_HEADER, buf)) kc_rt.preview_dirty = true;
        return;
    }

    uint8_t layer, start, count;
    if (!kc_next_dirty_chunk(&kc_rt, &layer, &start, &count)) return;
    buf[0]    = KC_SYNC_CHUNK;
    buf[1]    = layer;
    buf[2]    = start;
    buf[3]    = count;
    uint8_t n = kc_pack(&kc_store, layer, start, count, &buf[KC_SYNC_HEADER]);
    last_sync = timer_read32();
    if (!transaction_rpc_send(KEY_COLOURS_SYNC, KC_SYNC_HEADER + n, buf)) kc_mark_dirty(&kc_rt, layer, start, count);
}

// ---- QMK hooks ----

void keyboard_post_init_user(void) {
    kc_store_blank(&kc_store);
    kc_runtime_init(&kc_rt);
    transaction_register_rpc(KEY_COLOURS_SYNC, kc_sync_slave_handler);
    if (is_keyboard_master()) {
        if (eeconfig_is_user_datablock_valid()) eeconfig_read_user_datablock(&kc_store, 0, sizeof kc_store);
        kc_mark_all_dirty(&kc_rt);
    }
}

void housekeeping_task_user(void) {
    if (is_keyboard_master()) kc_sync_master();
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = kc_display_layer(&kc_rt, get_highest_layer(layer_state | default_layer_state), timer_read32());
    uint8_t val   = rgb_matrix_get_val();
    for (uint8_t i = led_min; i < led_max && i < KEY_COLOURS_LEDS; i++) {
        kc_rgb_t c;
        if (kc_get(&kc_store, layer, i, &c)) {
            kc_rgb_t s = kc_scale(c, val);
            rgb_matrix_set_color(i, s.r, s.g, s.b);
        }
    }
    return false;
}
