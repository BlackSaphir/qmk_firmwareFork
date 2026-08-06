// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"
#include "print.h"

void keyboard_post_init_user(void) {
    debug_enable = true;
    uprintf("=== KEYBOARD STARTED ===\n");
}

// Feuert bei JEDEM Tastendruck – testet, ob Console generell geht
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        uprintf("KEY! kc=%u row=%u col=%u\n", keycode, record->event.key.row, record->event.key.col);
    }
    return true;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    uprintf("ENCODER! index=%u cw=%d\n", index, clockwise);
    if (layer_state_is(U_NAV)) {
        tap_code(clockwise ? KC_VOLU : KC_VOLD);
    } else {
        tap_code(clockwise ? MS_WHLD : MS_WHLU);
    }
    return false;
}
