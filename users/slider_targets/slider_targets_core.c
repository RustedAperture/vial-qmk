#include "slider_targets_core.h"

void st_store_default(st_store_t *s) {
    memset(s, 0, sizeof(*s));
    s->count              = 1;
    s->current            = 0;
    s->targets[0].control = 62;
    s->targets[0].r       = 255;
    s->targets[0].g       = 255;
    s->targets[0].b       = 255;
    memcpy(s->targets[0].name, "Slider", 6);
}

bool st_store_valid(const st_store_t *s) {
    if (s->count < 1 || s->count > ST_MAX_TARGETS) return false;
    if (s->current >= s->count) return false;
    for (uint8_t i = 0; i < s->count; i++) {
        if (s->targets[i].control > 127) return false;
    }
    return true;
}

void st_runtime_init(st_runtime_t *rt) {
    memset(rt, 0, sizeof(*rt));
}

bool st_handle_hid(st_store_t *s, uint8_t *data, uint8_t length, st_change_t *change) {
    if (data[0] != SLIDER_TARGETS_CMD) return false;
    memset(change, 0, sizeof(*change));
    if (length < 32) {
        data[1] = ST_ERR;
        return true;
    }

    switch (data[1]) {
        case ST_GET_INFO:
            data[2] = ST_PROTOCOL_VERSION;
            data[3] = ST_MAX_TARGETS;
            data[4] = s->count;
            data[5] = s->current;
            data[6] = ST_NAME_MAX;
            break;
        case ST_GET: {
            uint8_t i = data[2];
            if (i >= s->count) {
                data[1] = ST_ERR;
                break;
            }
            data[3] = s->targets[i].control;
            data[4] = s->targets[i].r;
            data[5] = s->targets[i].g;
            data[6] = s->targets[i].b;
            memcpy(&data[7], s->targets[i].name, ST_NAME_MAX);
            break;
        }
        case ST_SET: {
            uint8_t i = data[2];
            if (i >= ST_MAX_TARGETS || data[3] > 127) {
                data[1] = ST_ERR;
                break;
            }
            s->targets[i].control = data[3];
            s->targets[i].r       = data[4];
            s->targets[i].g       = data[5];
            s->targets[i].b       = data[6];
            memcpy(s->targets[i].name, &data[7], ST_NAME_MAX);
            change->store_changed = true;
            change->first         = i;
            change->count         = 1;
            break;
        }
        case ST_SET_COUNT: {
            uint8_t n = data[2];
            if (n < 1 || n > ST_MAX_TARGETS) {
                data[1] = ST_ERR;
                break;
            }
            s->count = n;
            if (s->current >= n) s->current = 0;
            change->store_changed  = true;
            change->header_changed = true;
            break;
        }
        default:
            data[1] = ST_ERR;
            break;
    }
    return true;
}

void st_next(st_store_t *s, st_runtime_t *rt, uint32_t now) {
    s->current      = (uint8_t)((s->current + 1) % s->count);
    rt->flashing    = true;
    rt->flash_until = now + ST_FLASH_MS;
}

bool st_flash_active(st_runtime_t *rt, uint32_t now) {
    if (!rt->flashing) return false;
    if ((int32_t)(now - rt->flash_until) < 0) return true;
    rt->flashing = false;
    return false;
}

uint8_t st_scale(uint16_t raw) {
    uint32_t v = ((uint32_t)raw * 127 + ST_ADC_MAX / 2) / ST_ADC_MAX;
    return v > 127 ? 127 : (uint8_t)v;
}

bool st_slider_update(st_runtime_t *rt, uint16_t raw, uint8_t *value) {
    if (!rt->have_value) { // first reading only primes state
        rt->last_raw   = raw;
        rt->last_value = st_scale(raw);
        rt->have_value = true;
        return false;
    }
    uint16_t diff = raw > rt->last_raw ? raw - rt->last_raw : rt->last_raw - raw;
    bool     end  = raw == 0 || raw >= ST_ADC_MAX;
    if (diff < ST_HYSTERESIS && !(end && diff > 0)) return false;
    rt->last_raw  = raw;
    uint8_t v     = end ? (raw == 0 ? 0 : 127) : st_scale(raw);
    if (v == rt->last_value) return false;
    rt->last_value = v;
    *value         = v;
    return true;
}
