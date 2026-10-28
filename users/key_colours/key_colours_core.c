#include "key_colours_core.h"

#define KC_PACKET_LEN 32

void kc_store_blank(kc_store_t *s) { memset(s, 0, sizeof *s); }

void kc_runtime_init(kc_runtime_t *rt) {
    memset(rt, 0, sizeof *rt);
    rt->preview_layer = KC_PREVIEW_OFF;
}

bool kc_get(const kc_store_t *s, uint8_t layer, uint8_t led, kc_rgb_t *out) {
    if (layer >= KEY_COLOURS_LAYERS || led >= KEY_COLOURS_LEDS) return false;
    if (!(s->set_bits[layer][led >> 3] & (1u << (led & 7)))) return false;
    if (out) *out = s->rgb[layer][led];
    return true;
}

void kc_put(kc_store_t *s, uint8_t layer, uint8_t led, bool set, kc_rgb_t c) {
    if (layer >= KEY_COLOURS_LAYERS || led >= KEY_COLOURS_LEDS) return;
    if (set) {
        s->set_bits[layer][led >> 3] |= (uint8_t)(1u << (led & 7));
    } else {
        s->set_bits[layer][led >> 3] &= (uint8_t)~(1u << (led & 7));
    }
    s->rgb[layer][led] = c;
}

static bool range_ok(uint8_t layer, uint8_t start, uint8_t count) {
    return layer < KEY_COLOURS_LAYERS && count >= 1 && count <= KC_PER_PACKET &&
           (unsigned)start + count <= KEY_COLOURS_LEDS;
}

uint8_t kc_pack(const kc_store_t *s, uint8_t layer, uint8_t start, uint8_t count, uint8_t *out) {
    if (!range_ok(layer, start, count)) return 0;
    for (uint8_t i = 0; i < count; i++) {
        kc_rgb_t c = {0, 0, 0};
        bool set = kc_get(s, layer, (uint8_t)(start + i), &c);
        out[i * KC_ENTRY_SIZE + 0] = set ? 1 : 0;
        out[i * KC_ENTRY_SIZE + 1] = c.r;
        out[i * KC_ENTRY_SIZE + 2] = c.g;
        out[i * KC_ENTRY_SIZE + 3] = c.b;
    }
    return (uint8_t)(count * KC_ENTRY_SIZE);
}

bool kc_unpack(kc_store_t *s, uint8_t layer, uint8_t start, uint8_t count, const uint8_t *in) {
    if (!range_ok(layer, start, count)) return false;
    for (uint8_t i = 0; i < count; i++) {
        kc_rgb_t c = {in[i * KC_ENTRY_SIZE + 1], in[i * KC_ENTRY_SIZE + 2], in[i * KC_ENTRY_SIZE + 3]};
        kc_put(s, layer, (uint8_t)(start + i), in[i * KC_ENTRY_SIZE] != 0, c);
    }
    return true;
}

void kc_set_preview(kc_runtime_t *rt, uint8_t layer, uint32_t now) {
    if (layer >= KEY_COLOURS_LAYERS) {
        rt->preview_layer = KC_PREVIEW_OFF;
    } else {
        rt->preview_layer = layer;
        rt->preview_deadline = now + KC_PREVIEW_TIMEOUT_MS;
    }
    rt->preview_dirty = true;
}

uint8_t kc_display_layer(const kc_runtime_t *rt, uint8_t active_layer, uint32_t now) {
    if (rt->preview_layer == KC_PREVIEW_OFF) return active_layer;
    if ((int32_t)(now - rt->preview_deadline) >= 0) return active_layer;
    return rt->preview_layer;
}

kc_rgb_t kc_scale(kc_rgb_t c, uint8_t val) {
    kc_rgb_t o = {(uint8_t)((uint16_t)c.r * val / 255), (uint8_t)((uint16_t)c.g * val / 255),
                  (uint8_t)((uint16_t)c.b * val / 255)};
    return o;
}

/* round(255 * (v / 255)^2.2) */
static const uint8_t kc_gamma_table[256] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
      3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
      6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
     12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
     20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,
     30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,
     42,  43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,
     56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
     73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
     91,  93,  94,  95,  97,  98,  99, 100, 102, 103, 105, 106, 107, 109, 110, 111,
    113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135,
    137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161,
    163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190,
    192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
    223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255,
};

uint8_t kc_gamma(uint8_t v) {
    return kc_gamma_table[v];
}

kc_rgb_t kc_to_led(kc_rgb_t c, uint8_t val) {
    kc_rgb_t o = {(uint8_t)((uint16_t)kc_gamma(c.r) * KEY_COLOURS_BALANCE_R / 255),
                  (uint8_t)((uint16_t)kc_gamma(c.g) * KEY_COLOURS_BALANCE_G / 255),
                  (uint8_t)((uint16_t)kc_gamma(c.b) * KEY_COLOURS_BALANCE_B / 255)};
    return kc_scale(o, val);
}

void kc_mark_dirty(kc_runtime_t *rt, uint8_t layer, uint8_t start, uint8_t count) {
    if (layer >= KEY_COLOURS_LAYERS || count == 0 || start >= KEY_COLOURS_LEDS) return;
    unsigned last = (unsigned)start + count - 1;
    if (last >= KEY_COLOURS_LEDS) last = KEY_COLOURS_LEDS - 1;
    for (unsigned n = start / KC_PER_PACKET; n <= last / KC_PER_PACKET; n++) rt->dirty[layer] |= (1u << n);
}

void kc_mark_all_dirty(kc_runtime_t *rt) {
    for (uint8_t l = 0; l < KEY_COLOURS_LAYERS; l++) kc_mark_dirty(rt, l, 0, KEY_COLOURS_LEDS);
    rt->preview_dirty = true;
}

bool kc_next_dirty_chunk(kc_runtime_t *rt, uint8_t *layer, uint8_t *start, uint8_t *count) {
    for (uint8_t l = 0; l < KEY_COLOURS_LAYERS; l++) {
        if (!rt->dirty[l]) continue;
        for (uint8_t n = 0; n < 32; n++) {
            if (!(rt->dirty[l] & (1u << n))) continue;
            rt->dirty[l] &= ~(1u << n);
            unsigned st = (unsigned)n * KC_PER_PACKET;
            unsigned c = KEY_COLOURS_LEDS - st;
            *layer = l;
            *start = (uint8_t)st;
            *count = (uint8_t)(c > KC_PER_PACKET ? KC_PER_PACKET : c);
            return true;
        }
    }
    return false;
}

bool kc_handle_hid(kc_store_t *s, kc_runtime_t *rt, uint8_t *data, uint8_t length, uint32_t now,
                   kc_change_t *change) {
    if (change) memset(change, 0, sizeof *change);
    if (length < 1 || data[0] != KEY_COLOURS_CMD) return false;
    if (length < KC_PACKET_LEN) {
        if (length >= 2) data[1] = KC_ERR;
        return true;
    }
    kc_change_t scratch;
    if (!change) change = &scratch;
    memset(change, 0, sizeof *change);

    uint8_t layer = data[2], start = data[3], count = data[4];
    switch (data[1]) {
        case KC_GET_INFO:
            data[2] = KC_PROTOCOL_VERSION;
            data[3] = KEY_COLOURS_LAYERS;
            data[4] = (uint8_t)(KEY_COLOURS_LEDS & 0xFF);
            data[5] = (uint8_t)(KEY_COLOURS_LEDS >> 8);
            data[6] = s->initialised ? 1 : 0;
            break;
        case KC_GET:
            if (!range_ok(layer, start, count)) { data[1] = KC_ERR; break; }
            kc_pack(s, layer, start, count, &data[5]);
            break;
        case KC_SET: {
            if (!range_ok(layer, start, count)) { data[1] = KC_ERR; break; }
            bool was_init = s->initialised != 0;
            kc_unpack(s, layer, start, count, &data[5]);
            s->initialised = 1;
            kc_mark_dirty(rt, layer, start, count);
            change->store_changed = true;
            change->layer = layer;
            change->start = start;
            change->count = count;
            change->initialised_changed = !was_init;
            break;
        }
        case KC_PREVIEW:
            if (layer != KC_PREVIEW_OFF && layer >= KEY_COLOURS_LAYERS) { data[1] = KC_ERR; break; }
            kc_set_preview(rt, layer, now);
            break;
        default:
            data[1] = KC_ERR;
            break;
    }
    return true;
}
