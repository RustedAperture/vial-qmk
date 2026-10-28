// Pure-C core for the numpad slider's MIDI targets. No QMK dependencies.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SLIDER_TARGETS_CMD 0x56
#define ST_GET_INFO 0x01
#define ST_GET 0x02
#define ST_SET 0x03
#define ST_SET_COUNT 0x04
#define ST_ERR 0xFF
#define ST_PROTOCOL_VERSION 1
#define ST_MAX_TARGETS 8
#define ST_NAME_MAX 12
#define ST_FLASH_MS 1000

#ifndef ST_ADC_MAX
#    define ST_ADC_MAX 1023
#endif
// Raw ADC counts a reading must move from the last one before it is considered.
#ifndef ST_HYSTERESIS
#    define ST_HYSTERESIS 6
#endif

typedef struct __attribute__((packed)) {
    uint8_t control; // 0-127
    uint8_t r, g, b;
    char    name[ST_NAME_MAX]; // zero-padded, not necessarily terminated
} st_target_t;

typedef struct __attribute__((packed)) {
    uint8_t     count;   // 1-8
    uint8_t     current; // < count
    st_target_t targets[ST_MAX_TARGETS];
} st_store_t;

_Static_assert(sizeof(st_target_t) == 16, "target is 16 bytes");
_Static_assert(sizeof(st_store_t) == 130, "store is 130 bytes");

typedef struct {
    uint16_t last_raw; // last raw reading that passed hysteresis
    uint8_t  last_value;
    bool     have_value;
    uint32_t flash_until;
    bool     flashing;
} st_runtime_t;

// targets[first..first+count) and/or count/current changed
typedef struct {
    bool    store_changed;
    uint8_t first, count;
    bool    header_changed;
} st_change_t;

void    st_store_default(st_store_t *s);
bool    st_store_valid(const st_store_t *s);
void    st_runtime_init(st_runtime_t *rt);
bool    st_handle_hid(st_store_t *s, uint8_t *data, uint8_t length, st_change_t *change);
void    st_next(st_store_t *s, st_runtime_t *rt, uint32_t now);
// True while the flash from the last st_next is showing. Clears rt->flashing once it has expired,
// so it stays false (even after the 32-bit timer wraps) until the next st_next.
bool    st_flash_active(st_runtime_t *rt, uint32_t now);
uint8_t st_scale(uint16_t raw);
bool    st_slider_update(st_runtime_t *rt, uint16_t raw, uint8_t *value);
