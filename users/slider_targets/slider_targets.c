// QMK glue: keyboard EEPROM datablock, raw HID, slider MIDI and the target flash.
// Defines eeconfig_init_kb_datablock.
#include QMK_KEYBOARD_H
#include "analog.h"
#include "qmk_midi.h"
#include "key_colours.h"
#include "slider_targets.h"

_Static_assert(sizeof(st_store_t) == EECONFIG_KB_DATA_SIZE, "EECONFIG_KB_DATA_SIZE must match st_store_t");

#define ST_SLIDER_INTERVAL_MS 10
#define ST_MIDI_CHANNEL 2

_Static_assert(RGB_MATRIX_LED_COUNT <= 32, "flash_mask has one bit per LED");

static st_store_t   st_store;
static st_runtime_t st_rt;
static bool         st_loaded = false;

// ---- EEPROM ----

void eeconfig_init_kb_datablock(void) {
    st_store_default(&st_store);
    eeconfig_update_kb_datablock(&st_store, 0, sizeof st_store);
}

static void st_load(void) {
    if (st_loaded) return;
    st_loaded = true;
    st_runtime_init(&st_rt);
    if (eeconfig_is_kb_datablock_valid()) {
        eeconfig_read_kb_datablock(&st_store, 0, sizeof st_store);
        if (st_store_valid(&st_store)) return;
    }
    eeconfig_init_kb_datablock();
}

static void st_save_header(void) {
    // count and current are the bytes before targets
    eeconfig_update_kb_datablock(&st_store, 0, offsetof(st_store_t, targets));
}

static void st_save_flash(void) {
    eeconfig_update_kb_datablock(&st_store.flash_mask, offsetof(st_store_t, flash_mask), sizeof st_store.flash_mask);
}

static void st_save_change(const st_change_t *ch) {
    if (ch->flash_changed) st_save_flash();
    if (ch->header_changed) st_save_header();
    if (ch->count) {
        eeconfig_update_kb_datablock(&st_store.targets[ch->first], offsetof(st_store_t, targets) + (uint32_t)ch->first * sizeof(st_target_t), (uint32_t)ch->count * sizeof(st_target_t));
    }
}

// ---- QMK-facing functions ----

bool slider_targets_raw_hid(uint8_t *data, uint8_t length) {
    st_load();
    st_change_t ch;
    if (!st_handle_hid(&st_store, data, length, &ch)) return false;
    if (ch.store_changed) st_save_change(&ch);
    return true;
}

bool slider_targets_process_record(uint16_t keycode, keyrecord_t *record) {
    if (keycode != SLIDER_NEXT) return true;
    if (record->event.pressed) {
        st_load();
        st_next(&st_store, &st_rt, timer_read32());
        st_save_header();
    }
    return false;
}

void slider_targets_task(void) {
    static uint32_t last_read = 0;
    st_load();
    if (timer_elapsed32(last_read) < ST_SLIDER_INTERVAL_MS) return;
    last_read = timer_read32();
    uint8_t value;
    if (st_slider_update(&st_rt, (uint16_t)analogReadPin(SLIDER_PIN), &value)) {
        midi_send_cc(&midi_device, ST_MIDI_CHANNEL, st_store.targets[st_store.current].control, value);
    }
}

void slider_targets_render(uint8_t led_min, uint8_t led_max) {
    st_load();
    if (!st_flash_active(&st_rt, timer_read32())) return;
    const st_target_t *t = &st_store.targets[st_store.current];
    kc_rgb_t           c = key_colours_to_led((kc_rgb_t){t->r, t->g, t->b});
    for (uint8_t led = led_min; led < led_max && led < 32; led++) {
        if (st_store.flash_mask & (1UL << led)) rgb_matrix_set_color(led, c.r, c.g, c.b);
    }
}
