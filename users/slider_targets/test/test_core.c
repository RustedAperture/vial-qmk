#include <assert.h>
#include <stdio.h>
#include "slider_targets_core.h"

static void req(uint8_t *d, uint8_t sub) {
    memset(d, 0, 32);
    d[0] = SLIDER_TARGETS_CMD;
    d[1] = sub;
}

static void test_default(void) {
    st_store_t s;
    st_store_default(&s);
    assert(s.count == 1 && s.current == 0);
    assert(s.targets[0].control == 62);
    assert(s.targets[0].r == 255 && s.targets[0].g == 255 && s.targets[0].b == 255);
    assert(memcmp(s.targets[0].name, "Slider\0\0\0\0\0\0", 12) == 0);
    assert(s.flash_mask == ST_DEFAULT_FLASH_MASK && s.flash_mask == 0xFFFFFFFFu);
    assert(sizeof(st_store_t) == 134 && offsetof(st_store_t, flash_mask) == 130);
    assert(st_store_valid(&s));
    s.count = 0;
    assert(!st_store_valid(&s));
    s.count = 9;
    assert(!st_store_valid(&s));
    s.count = 2;
    s.current = 2;
    assert(!st_store_valid(&s));
    s.current = 0;
    s.targets[1].control = 128;
    assert(!st_store_valid(&s));
    s.count = 1; // entries beyond count are ignored
    assert(st_store_valid(&s));
}

static void test_protocol(void) {
    st_store_t s;
    st_change_t ch;
    uint8_t d[32];
    st_store_default(&s);

    req(d, ST_GET_INFO);
    assert(st_handle_hid(&s, d, 32, &ch));
    assert(d[2] == 2 && d[3] == 8 && d[4] == 1 && d[5] == 0 && d[6] == 12);
    assert(!ch.store_changed);

    d[0] = 0x55;
    assert(!st_handle_hid(&s, d, 32, &ch));

    req(d, ST_SET);
    d[2] = 1; d[3] = 7; d[4] = 1; d[5] = 2; d[6] = 3;
    memcpy(&d[7], "Reverb\0\0\0\0\0\0", 12);
    assert(st_handle_hid(&s, d, 32, &ch));
    assert(d[1] == ST_SET);
    assert(ch.store_changed && ch.first == 1 && ch.count == 1 && !ch.header_changed);

    req(d, ST_SET_COUNT);
    d[2] = 2;
    assert(st_handle_hid(&s, d, 32, &ch));
    assert(d[1] == ST_SET_COUNT && d[2] == 2);
    assert(ch.store_changed && ch.header_changed);
    assert(s.count == 2);

    req(d, ST_GET);
    d[2] = 1;
    assert(st_handle_hid(&s, d, 32, &ch));
    assert(!ch.store_changed);
    assert(d[2] == 1 && d[3] == 7 && d[4] == 1 && d[5] == 2 && d[6] == 3);
    assert(memcmp(&d[7], "Reverb\0\0\0\0\0\0", 12) == 0);
}

static void test_flash(void) {
    st_store_t s;
    st_change_t ch;
    uint8_t d[32];
    st_store_default(&s);

    req(d, ST_GET_FLASH);
    assert(st_handle_hid(&s, d, 32, &ch));
    assert(d[1] == ST_GET_FLASH && !ch.store_changed && !ch.flash_changed);
    assert(d[2] == 0xFF && d[3] == 0xFF && d[4] == 0xFF && d[5] == 0xFF);

    req(d, ST_SET_FLASH);
    d[2] = 0x78; d[3] = 0x56; d[4] = 0x34; d[5] = 0x12;
    assert(st_handle_hid(&s, d, 32, &ch));
    assert(d[1] == ST_SET_FLASH && d[2] == 0x78 && d[5] == 0x12);
    assert(s.flash_mask == 0x12345678u);
    assert(ch.store_changed && ch.flash_changed && !ch.header_changed && ch.count == 0);

    req(d, ST_GET_FLASH);
    assert(st_handle_hid(&s, d, 32, &ch));
    assert(d[2] == 0x78 && d[3] == 0x56 && d[4] == 0x34 && d[5] == 0x12);
    assert(!ch.flash_changed);

    // short packet: error, mask untouched
    req(d, ST_SET_FLASH); d[2] = 1;
    assert(st_handle_hid(&s, d, 31, &ch) && d[1] == ST_ERR && !ch.flash_changed);
    assert(s.flash_mask == 0x12345678u);

    // the mask does not affect validity
    s.flash_mask = 0;
    assert(st_store_valid(&s));
}

static void test_bad_args(void) {
    st_store_t s, ref;
    st_change_t ch;
    uint8_t d[32];
    st_store_default(&s);
    ref = s;

    req(d, ST_GET); d[2] = 1;
    assert(st_handle_hid(&s, d, 32, &ch) && d[1] == ST_ERR && !ch.store_changed);
    req(d, ST_SET); d[2] = 8; d[3] = 1;
    assert(st_handle_hid(&s, d, 32, &ch) && d[1] == ST_ERR && !ch.store_changed);
    req(d, ST_SET); d[2] = 0; d[3] = 128;
    assert(st_handle_hid(&s, d, 32, &ch) && d[1] == ST_ERR && !ch.store_changed);
    req(d, ST_SET_COUNT); d[2] = 0;
    assert(st_handle_hid(&s, d, 32, &ch) && d[1] == ST_ERR && !ch.store_changed);
    req(d, ST_SET_COUNT); d[2] = 9;
    assert(st_handle_hid(&s, d, 32, &ch) && d[1] == ST_ERR && !ch.store_changed);
    req(d, 0x77);
    assert(st_handle_hid(&s, d, 32, &ch) && d[1] == ST_ERR && !ch.store_changed);
    req(d, ST_GET_INFO);
    assert(st_handle_hid(&s, d, 31, &ch) && d[1] == ST_ERR && !ch.store_changed);
    assert(memcmp(&s, &ref, sizeof s) == 0);
}

static void test_count_resets_current(void) {
    st_store_t s;
    st_change_t ch;
    uint8_t d[32];
    st_runtime_t rt;
    st_store_default(&s);
    st_runtime_init(&rt);
    s.count = 3;
    s.current = 2;
    req(d, ST_SET_COUNT); d[2] = 2;
    assert(st_handle_hid(&s, d, 32, &ch) && d[1] == ST_SET_COUNT);
    assert(s.count == 2 && s.current == 0);
    assert(st_store_valid(&s));
}

static void test_next_flash(void) {
    st_store_t s;
    st_runtime_t rt;
    st_store_default(&s);
    s.count = 2;
    st_runtime_init(&rt);
    assert(!st_flash_active(&rt, 0));
    st_next(&s, &rt, 100);
    assert(s.current == 1 && st_flash_active(&rt, 100));
    assert(st_flash_active(&rt, 1099) && !st_flash_active(&rt, 1100));
    st_next(&s, &rt, 600); // restart during flash
    assert(s.current == 0 && st_flash_active(&rt, 1599) && !st_flash_active(&rt, 1600));
    // wrap-safe across the 32-bit timer rollover
    st_next(&s, &rt, 0xFFFFF000u);
    assert(st_flash_active(&rt, 0xFFFFF000u));
    assert(st_flash_active(&rt, 0xFFFFF000u + 999u));
    assert(!st_flash_active(&rt, 0xFFFFF000u + 1000u));
    assert(!st_flash_active(&rt, 0x10000u));
    // st_next with a single target stays at 0 but still flashes
    s.count = 1; s.current = 0;
    st_next(&s, &rt, 5);
    assert(s.current == 0 && st_flash_active(&rt, 5));
}

// once expired, the flash stays off even when the timer comes back round 2^31 ms later
static void test_flash_stays_off(void) {
    st_store_t s;
    st_runtime_t rt;
    st_store_default(&s);
    st_runtime_init(&rt);
    st_next(&s, &rt, 1000);
    assert(st_flash_active(&rt, 1999));
    assert(!st_flash_active(&rt, 2000));
    assert(!st_flash_active(&rt, 1000u + 0x80000000u + 5u));
    assert(!st_flash_active(&rt, 2000u + 0x80000000u + 5u)); // 2^31 ms past the end: the old check said true
}

static void test_scale(void) {
    assert(st_scale(0) == 0);
    assert(st_scale(1023) == 127);
    assert(st_scale(512) == 64);
    assert(st_scale(5000) == 127);
}

static void test_slider(void) {
    st_runtime_t rt;
    uint8_t v = 255;
    st_runtime_init(&rt);
    assert(!st_slider_update(&rt, 500, &v)); // primes
    assert(!st_slider_update(&rt, 503, &v));
    assert(!st_slider_update(&rt, 497, &v));
    assert(!st_slider_update(&rt, 500, &v));
    assert(v == 255);
    assert(st_slider_update(&rt, 520, &v));
    assert(v == st_scale(520));
    assert(!st_slider_update(&rt, 520, &v));
    assert(!st_slider_update(&rt, 523, &v));
    // end zones: reach 0 and 127 from inside the hysteresis band
    st_runtime_init(&rt);
    assert(!st_slider_update(&rt, 5, &v)); // primes
    assert(st_slider_update(&rt, 2, &v) && v == 0);
    assert(!st_slider_update(&rt, 0, &v)); // already 0
    assert(!st_slider_update(&rt, 3, &v));
    st_runtime_init(&rt);
    assert(!st_slider_update(&rt, 1015, &v)); // primes
    assert(st_slider_update(&rt, 1019, &v) && v == 127);
    assert(!st_slider_update(&rt, 1023, &v)); // repeated end readings send nothing
    assert(!st_slider_update(&rt, 1020, &v));
    // leaving the end zone works normally
    assert(st_slider_update(&rt, 900, &v) && v == st_scale(900));
    st_runtime_init(&rt);
    assert(!st_slider_update(&rt, 20, &v));
    assert(st_slider_update(&rt, 7, &v) && v == st_scale(7));
    assert(st_slider_update(&rt, 3, &v) && v == 0);
}

int main(void) {
    test_default();
    test_protocol();
    test_flash();
    test_bad_args();
    test_count_resets_current();
    test_next_flash();
    test_flash_stays_off();
    test_scale();
    test_slider();
    puts("all slider core tests passed");
    return 0;
}
