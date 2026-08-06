// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

#include QMK_KEYBOARD_H
#include "manna-harbour_miryoku.h"

// ===========================================================================
//  Encoder
// ===========================================================================
// ENCODER_ENABLE ist nur im Encoder-Build gesetzt. Der Guard verhindert eine
// ungenutzte Funktion im Display-Build.

#ifdef ENCODER_ENABLE
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (layer_state_is(U_NAV) || layer_state_is(U_MEDIA)) {
        tap_code(clockwise ? KC_VOLU : KC_VOLD);   // Lautstaerke
    } else if (layer_state_is(U_NUM)) {
        tap_code(clockwise ? KC_PGDN : KC_PGUP);   // Seitenweise scrollen
    } else {
        tap_code(clockwise ? MS_WHLU : MS_WHLD);   // Mausrad
    }
    // Drehrichtung tauschen: die beiden Keycodes im jeweiligen tap_code() tauschen
    return false;
}
#endif

// ===========================================================================
//  Halcyon TFT-Display (Quantum Painter / ST7789)
// ===========================================================================
// HLC_TFT_DISPLAY wird von users/halcyon_modules/splitkb/hlc_tft_display/config.h
// gesetzt, sobald mit -e HLC_TFT_DISPLAY=1 gebaut wird.
//
// Warum ueberhaupt eigener Code?
// splitkb zeichnet die aktive Ebene als Ziffern-Grafik und kennt nur 0-7.
// Miryoku hat aber ZEHN Ebenen (U_BASE=0 ... U_FUN=9), d.h. Sym und Fun
// wuerden rot als "undef" erscheinen. Deshalb wird hier statt der Ziffern der
// Miryoku-Ebenenname als Text gerendert.

#ifdef HLC_TFT_DISPLAY

#    include "halcyon.h"
#    include "hlc_tft_display.h"
#    include "graphics/fonts/Retron2000-27.qff.h"
#    include "graphics/fonts/Retron2000-underline-27.qff.h"

// splitkb definiert nur HSV_LAYER_0 .. HSV_LAYER_7
#    define HSV_LAYER_8 213, 56, 255
#    define HSV_LAYER_9 96, 128, 255

// Die Namen kommen direkt aus MIRYOKU_LAYER_LIST, damit sie automatisch
// stimmen - auch wenn du die Ebenenliste spaeter aenderst.
static const char *const miryoku_layer_names[] = {
#    define MIRYOKU_X(LAYER, STRING) [U_##LAYER] = STRING,
    MIRYOKU_LAYER_LIST
#    undef MIRYOKU_X
};

static const uint8_t miryoku_layer_hsv[][3] = {
    [U_BASE]   = {HSV_LAYER_0},
    [U_EXTRA]  = {HSV_LAYER_1},
    [U_TAP]    = {HSV_LAYER_2},
    [U_BUTTON] = {HSV_LAYER_3},
    [U_NAV]    = {HSV_LAYER_4},
    [U_MOUSE]  = {HSV_LAYER_5},
    [U_MEDIA]  = {HSV_LAYER_6},
    [U_NUM]    = {HSV_LAYER_7},
    [U_SYM]    = {HSV_LAYER_8},
    [U_FUN]    = {HSV_LAYER_9},
};

static const char *const lock_labels[3] = {"Caps", "Num", "Scroll"};

static const uint8_t lock_hsv_off[3][3] = {{HSV_CAPS_OFF}, {HSV_NUM_OFF}, {HSV_SCROLL_OFF}};
static const uint8_t lock_hsv_on[3][3]  = {{HSV_CAPS_ON}, {HSV_NUM_ON}, {HSV_SCROLL_ON}};

static painter_font_handle_t hlc_font    = NULL;
static painter_font_handle_t hlc_font_ul = NULL;

// Wird von halcyon.c am Anfang von display_module_housekeeping_task_kb()
// aufgerufen. Rueckgabe false => die splitkb-Standardausgabe wird komplett
// uebersprungen, wir zeichnen und flushen selbst.
bool display_module_housekeeping_task_user(bool second_display) {
    // Zweites Display (Slave-Haelfte, wenn der Master ebenfalls ein Display
    // hat): splitkb-Standard (Game of Life) weiterlaufen lassen.
    if (second_display) {
        return true;
    }

    static bool    initialised = false;
    static uint8_t last_layer  = 0xFF;
    static led_t   last_locks  = {0};

    bool dirty = false;

    if (!initialised) {
        hlc_font    = qp_load_font_mem(font_Retron2000_27);
        hlc_font_ul = qp_load_font_mem(font_Retron2000_underline_27);
        initialised = true;
        dirty       = true;
    }
    if (hlc_font == NULL || hlc_font_ul == NULL) {
        return false; // Font nicht ladbar - nichts zeichnen, aber kb-Teil auslassen
    }

    // ---------------------------------------------------------------- Ebene
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer >= ARRAY_SIZE(miryoku_layer_names) || layer >= ARRAY_SIZE(miryoku_layer_hsv) || miryoku_layer_names[layer] == NULL) {
        layer = U_BASE;
    }

    if (layer != last_layer || dirty) {
        // Alte Zeile loeschen, Namen sind unterschiedlich lang
        qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, 5 + hlc_font->line_height, HSV_BLACK, true);
        qp_drawtext_recolor(lcd_surface, 5, 5, hlc_font, miryoku_layer_names[layer],
                            miryoku_layer_hsv[layer][0], miryoku_layer_hsv[layer][1], miryoku_layer_hsv[layer][2],
                            HSV_BLACK);
        last_layer = layer;
        dirty      = true;
    }

    // ------------------------------------------------- Caps / Num / Scroll
    led_t locks = host_keyboard_led_state();
    if (locks.raw != last_locks.raw || dirty) {
        const bool state[3] = {locks.caps_lock, locks.num_lock, locks.scroll_lock};

        for (uint8_t i = 0; i < 3; i++) {
            uint16_t y = LCD_HEIGHT - hlc_font->line_height * (3 - i) - (15 - i * 5);

            qp_rect(lcd_surface, 0, y, LCD_WIDTH - 1, y + hlc_font->line_height, HSV_BLACK, true);

            const uint8_t *c = state[i] ? lock_hsv_on[i] : lock_hsv_off[i];
            qp_drawtext_recolor(lcd_surface, 5, y, state[i] ? hlc_font_ul : hlc_font,
                                lock_labels[i], c[0], c[1], c[2], HSV_BLACK);
        }

        last_locks = locks;
        dirty      = true;
    }

    // -------------------------------------------------- Surface -> Display
    // Nur bei tatsaechlicher Aenderung flushen, das spart SPI-Bandbreite.
    if (dirty) {
        qp_surface_draw(lcd_surface, lcd, 0, 0, 0);
        qp_flush(lcd);
    }

    return false; // wir haben die Master-Anzeige vollstaendig uebernommen
}

#endif // HLC_TFT_DISPLAY
