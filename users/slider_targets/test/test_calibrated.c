// Calibrated-range cases; built with -DST_ADC_MIN=16 -DST_ADC_MAX=940.
#include <assert.h>
#include <stdio.h>
#include "slider_targets_core.h"

int main(void) {
    assert(st_scale(16) == 0);
    assert(st_scale(940) == 127);
    assert(st_scale(478) == 64); // (462 * 127 + 462) / 924 = 64
    assert(st_scale(5) == 0);
    assert(st_scale(1000) == 127);

    st_runtime_t rt;
    st_runtime_init(&rt);
    uint8_t v = 99;
    assert(!st_slider_update(&rt, 300, &v)); // primes
    for (int i = 0; i < 10; i++) { // end zone: first reading sends 0
        bool sent = st_slider_update(&rt, 10, &v);
        assert(sent == (i == 0));
        if (sent) assert(v == 0);
    }
    assert(st_slider_update(&rt, 950, &v) && v == 127);
    assert(!st_slider_update(&rt, 945, &v));
    puts("calibrated slider core tests passed");
    return 0;
}
