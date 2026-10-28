#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "key_colours_core.h"

static kc_rgb_t rgb(uint8_t r, uint8_t g, uint8_t b) { kc_rgb_t c = {r, g, b}; return c; }
static bool eq(kc_rgb_t a, kc_rgb_t b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

static void test_blank_store_is_unset_and_uninitialised(void) {
    kc_store_t s; kc_rgb_t c;
    memset(&s, 0xAA, sizeof s);
    kc_store_blank(&s);
    assert(s.initialised == 0);
    for (uint8_t l = 0; l < KEY_COLOURS_LAYERS; l++)
        for (uint8_t i = 0; i < KEY_COLOURS_LEDS; i++) assert(!kc_get(&s, l, i, &c));
}

static void test_put_get_roundtrip(void) {
    kc_store_t s; kc_rgb_t c;
    kc_store_blank(&s);
    kc_put(&s, 1, 13, true, rgb(10, 20, 30));
    assert(kc_get(&s, 1, 13, &c));
    assert(eq(c, rgb(10, 20, 30)));
    assert(!kc_get(&s, 1, 12, &c));
    kc_put(&s, 1, 13, false, rgb(0, 0, 0));
    assert(!kc_get(&s, 1, 13, &c));
}

static void test_get_info_reply(void) {
    kc_store_t s; kc_runtime_t rt; kc_change_t ch;
    uint8_t d[32] = {KEY_COLOURS_CMD, KC_GET_INFO};
    kc_store_blank(&s); kc_runtime_init(&rt);
    assert(kc_handle_hid(&s, &rt, d, 32, 0, &ch));
    assert(d[1] == KC_GET_INFO);
    assert(d[2] == 1 && d[3] == 2 && d[4] == 14 && d[5] == 0 && d[6] == 0);
}

static void test_set_then_get_over_hid(void) {
    kc_store_t s; kc_runtime_t rt; kc_change_t ch;
    uint8_t d[32] = {KEY_COLOURS_CMD, KC_SET, 0, 4, 2, 1, 255, 0, 0, 0, 0, 0, 0};
    kc_store_blank(&s); kc_runtime_init(&rt);
    assert(kc_handle_hid(&s, &rt, d, 32, 0, &ch));
    assert(ch.store_changed && ch.layer == 0 && ch.start == 4 && ch.count == 2);
    assert(ch.initialised_changed);

    uint8_t g[32] = {KEY_COLOURS_CMD, KC_GET, 0, 4, 2};
    assert(kc_handle_hid(&s, &rt, g, 32, 0, &ch));
    assert(!ch.store_changed);
    uint8_t want[8] = {1, 255, 0, 0, 0, 0, 0, 0};
    assert(memcmp(&g[5], want, 8) == 0);

    uint8_t i[32] = {KEY_COLOURS_CMD, KC_GET_INFO};
    kc_handle_hid(&s, &rt, i, 32, 0, &ch);
    assert(i[6] == 1);
}

static void check_bad(kc_store_t *s, kc_runtime_t *rt, uint8_t layer, uint8_t start, uint8_t count) {
    kc_store_t before = *s; kc_change_t ch;
    uint8_t d[32] = {KEY_COLOURS_CMD, KC_SET, layer, start, count, 1, 9, 9, 9};
    assert(kc_handle_hid(s, rt, d, 32, 0, &ch));
    assert(d[1] == KC_ERR);
    assert(!ch.store_changed);
    assert(memcmp(&before, s, sizeof before) == 0);
}

static void test_bad_args_rejected(void) {
    kc_store_t s; kc_runtime_t rt; kc_change_t ch;
    kc_store_blank(&s); kc_runtime_init(&rt);
    check_bad(&s, &rt, 2, 0, 1);
    check_bad(&s, &rt, 0, 12, 3);
    check_bad(&s, &rt, 0, 0, 0);
    check_bad(&s, &rt, 0, 0, 7);
    uint8_t p[32] = {KEY_COLOURS_CMD, KC_PREVIEW, 2};
    assert(kc_handle_hid(&s, &rt, p, 32, 0, &ch));
    assert(p[1] == KC_ERR);
    uint8_t shortp[32] = {KEY_COLOURS_CMD, KC_GET_INFO};
    assert(kc_handle_hid(&s, &rt, shortp, 31, 0, &ch));
    assert(shortp[1] == KC_ERR);
}

static void test_unknown_command_not_handled(void) {
    kc_store_t s; kc_runtime_t rt; kc_change_t ch;
    uint8_t d[32] = {0x54, KC_GET_INFO};
    kc_store_blank(&s); kc_runtime_init(&rt);
    assert(!kc_handle_hid(&s, &rt, d, 32, 0, &ch));
}

static void test_preview_expires(void) {
    kc_store_t s; kc_runtime_t rt; kc_change_t ch;
    kc_store_blank(&s); kc_runtime_init(&rt);
    assert(kc_display_layer(&rt, 0, 5) == 0);
    uint8_t d[32] = {KEY_COLOURS_CMD, KC_PREVIEW, 1};
    assert(kc_handle_hid(&s, &rt, d, 32, 1000, &ch));
    assert(d[1] == KC_PREVIEW && rt.preview_dirty);
    assert(kc_display_layer(&rt, 0, 3999) == 1);
    assert(kc_display_layer(&rt, 0, 4000) == 0);
    uint8_t off[32] = {KEY_COLOURS_CMD, KC_PREVIEW, 0xFF};
    kc_handle_hid(&s, &rt, d, 32, 1000, &ch);
    kc_handle_hid(&s, &rt, off, 32, 1100, &ch);
    assert(kc_display_layer(&rt, 0, 1101) == 0);
    kc_set_preview(&rt, 1, 0xFFFFF000u);
    assert(kc_display_layer(&rt, 0, 0xFFFFF000u + 2999u) == 1);
    assert(kc_display_layer(&rt, 0, 0xFFFFF000u + 3000u) == 0);
}

static void test_scale(void) {
    kc_rgb_t c = kc_scale(rgb(255, 128, 0), 127);
    assert(eq(c, rgb(127, 63, 0)));
    assert(eq(kc_scale(rgb(255, 128, 0), 255), rgb(255, 128, 0)));
    assert(eq(kc_scale(rgb(255, 128, 7), 0), rgb(0, 0, 0)));
}

static void test_gamma(void) {
    assert(kc_gamma(0) == 0);
    assert(kc_gamma(255) == 255);
    assert(kc_gamma(128) == 56);
    assert(kc_gamma(200) == 149);
    assert(kc_gamma(83) == 22);
    assert(kc_gamma(1) == 0);
    assert(kc_gamma(254) == 253);
}

/* test build sets KEY_COLOURS_BALANCE_G=140, KEY_COLOURS_BALANCE_B=95 (red defaults to 255) */
static void test_to_led(void) {
    assert(eq(kc_to_led(rgb(255, 255, 255), 255), rgb(255, 140, 95)));
    assert(eq(kc_to_led(rgb(0, 200, 83), 255), rgb(0, 81, 8)));
    assert(eq(kc_to_led(rgb(255, 23, 68), 255), rgb(255, 0, 5)));
    assert(eq(kc_to_led(rgb(255, 255, 255), 127), rgb(127, 69, 47)));
    assert(eq(kc_to_led(rgb(255, 255, 255), 0), rgb(0, 0, 0)));
}

static void test_dirty_chunks(void) {
    kc_runtime_t rt; uint8_t l, st, n;
    kc_runtime_init(&rt);
    assert(!kc_next_dirty_chunk(&rt, &l, &st, &n));
    kc_mark_dirty(&rt, 1, 5, 3);
    assert(kc_next_dirty_chunk(&rt, &l, &st, &n) && l == 1 && st == 0 && n == 6);
    assert(kc_next_dirty_chunk(&rt, &l, &st, &n) && l == 1 && st == 6 && n == 6);
    assert(!kc_next_dirty_chunk(&rt, &l, &st, &n));
    kc_mark_dirty(&rt, 0, 13, 1);
    assert(kc_next_dirty_chunk(&rt, &l, &st, &n) && l == 0 && st == 12 && n == 2);
    kc_mark_all_dirty(&rt);
    assert(rt.preview_dirty);
    int cnt = 0;
    while (kc_next_dirty_chunk(&rt, &l, &st, &n)) cnt++;
    assert(cnt == 2 * 3);
}

int main(void) {
    test_blank_store_is_unset_and_uninitialised();
    test_put_get_roundtrip();
    test_get_info_reply();
    test_set_then_get_over_hid();
    test_bad_args_rejected();
    test_unknown_command_not_handled();
    test_preview_expires();
    test_scale();
    test_gamma();
    test_to_led();
    test_dirty_chunks();
    puts("all core tests passed");
    return 0;
}
