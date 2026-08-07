// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku
//
// Bongo Cat: Bilddaten aus https://github.com/dancarroll/qmk-bongo (GPL-2.0),
// urspruenglich von github.com/j-inc, Grafik von @pixelbenny. Die dortigen
// 128x32-OLED-Arrays werden hier unveraendert uebernommen und auf die
// Quantum-Painter-Surface gezeichnet statt per oled_write_raw_P.

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

// ---------------------------------------------------------------------------
//  Farben der Ebenennamen
// ---------------------------------------------------------------------------
// Je eine Zeile {Hue, Saturation, Value}, alle drei Werte 0-255.
// Hue: 0 = Rot, 43 = Gelb, 85 = Gruen, 128 = Cyan, 170 = Blau, 213 = Magenta.
// Saturation 255 = volle Farbe, 0 = Weiss. Value = Helligkeit.
//
// Die Farbtoene sind hier bewusst gleichmaessig ueber den Kreis verteilt,
// damit sich die zehn Ebenen auf einen Blick unterscheiden lassen. Base
// bleibt neutral weiss, weil es der Ruhezustand ist.
// splitkbs Vorgabewerte hiessen HSV_LAYER_0 .. HSV_LAYER_7 und lagen alle
// dicht beieinander - falls du sie zurueck willst, einfach wieder eintragen.

static const uint8_t miryoku_layer_hsv[][3] = {
    [U_BASE]   = { 21, 255, 255},  // Orange
    [U_EXTRA]  = {  0, 255, 255},  // Rot
    [U_TAP]    = {  0,   0, 200},  // Weiss
    [U_BUTTON] = { 43, 255, 255},  // Gelb
    [U_NAV]    = { 85, 255, 255},  // Gruen
    [U_MOUSE]  = {106, 255, 255},  // Tuerkis
    [U_MEDIA]  = {128, 255, 255},  // Cyan
    [U_NUM]    = {170, 255, 255},  // Blau
    [U_SYM]    = {191, 255, 255},  // Violett
    [U_FUN]    = {224, 255, 255},  // Magenta
};

// Die Namen kommen direkt aus MIRYOKU_LAYER_LIST, damit sie automatisch
// stimmen - auch wenn du die Ebenenliste spaeter aenderst.
static const char *const miryoku_layer_names[] = {
#    define MIRYOKU_X(LAYER, STRING) [U_##LAYER] = STRING,
    MIRYOKU_LAYER_LIST
#    undef MIRYOKU_X
};

static const char *const lock_labels[3] = {"Caps", "Num", "Scroll"};

static const uint8_t lock_hsv_off[3][3] = {{HSV_CAPS_OFF}, {HSV_NUM_OFF}, {HSV_SCROLL_OFF}};
static const uint8_t lock_hsv_on[3][3]  = {{HSV_CAPS_ON}, {HSV_NUM_ON}, {HSV_SCROLL_ON}};

static painter_font_handle_t hlc_font    = NULL;
static painter_font_handle_t hlc_font_ul = NULL;

// ---------------------------------------------------------------------------
//  Bongo Cat
// ---------------------------------------------------------------------------
// Format wie beim SSD1306: 4 Seiten a 128 Spalten, ein Byte = 8 uebereinander
// liegende Pixel, Bit 0 oben. Pixel (x,y) liegt also in
// frame[(y / 8) * 128 + x], Bit (y % 8).

#    define BONGO_W 128
#    define BONGO_H 32
#    define BONGO_X ((LCD_WIDTH - BONGO_W) / 2)
#    define BONGO_Y ((LCD_HEIGHT - BONGO_H) / 2)
#    define BONGO_FRAME_MS 200          // Bilddauer, wie im Original
#    define HSV_BONGO 0, 0, 255         // Fellfarbe: weiss

// Normalerweise kommen diese beiden aus der config.h, weil dort auch
// HLC_BACKLIGHT_TIMEOUT daraus abgeleitet wird. Die Fallbacks sorgen nur
// dafuer, dass die keymap.c allein uebersetzbar bleibt.
// ACHTUNG: ohne den Block in der config.h schaltet das Panel bereits nach
// 120 s ab - also genau dann, wenn die Katze erscheinen soll. Man saehe sie
// dann nie.
#    ifndef HLC_IDLE_ANIM_START
#        define HLC_IDLE_ANIM_START 120000
#    endif
#    ifndef HLC_IDLE_ANIM_DURATION
#        define HLC_IDLE_ANIM_DURATION 120000
#    endif

enum bongo_frame {
    BONGO_WAITING,  // beide Pfoten auf dem Tisch
    BONGO_READY,    // beide Pfoten in der Luft
    BONGO_TAP_L,
    BONGO_TAP_R,
    BONGO_FRAME_COUNT,
};

static const uint8_t bongo_frames[BONGO_FRAME_COUNT][BONGO_W * BONGO_H / 8] = {
    // [BONGO_WAITING]
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128,   0,   0,   0,   0,   0, 128,  64,  64,  32,  32,  32,
         32,  16,  16,  16,  16,   8,   4,   2,   1,   1,   2,  12,  48,  64, 128,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 128, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,  30, 225,   0,   0,   1,   1,   2,   2,   1,   0,   0,   0,   0, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0, 128,   0,  48,  48,   0,   0,   1,
          1,   2,   4,   8,  16,  32,  64, 128,   0,   0,   0, 128, 128, 128, 128,  64,
         64,  64,  64,  32,  32,  32,  32,  16,  16,  16,  16,   8,   8,   8,   8,   8,
          4,   4,   4,   4,   4,   2,   2,   2,   2,   1,   1,   1,   1,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        128, 112,  12,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
          0,  64, 160,  33,  34,  18,  17,  17,  17,   9,   8,   8,   8,   8,   4,   4,
          8,   8,  16,  16,  16,  16,  16,  17,  15,   1,   1,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128, 128, 128, 128,  64,  64,  64,  64,  32,  32,  32,  32,
         16,  16,  16,  16,  16,   8,   8,   8,   8,   8,   4,   4,   4,   4,   4,   2,
          3,   2,   2,   1,   1,   1,   1,   1,   1,   2,   2,   4,   4,   8,   8,   8,
          8,   8,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    },
    // [BONGO_READY]
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128,   0,   0,   0,   0,   0, 128,  64,  64,  32,  32,  32,
         32,  16,  16,  16,  16,   8,   4,   2,   1,   1,   2,  12,  48,  64, 128,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 128, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,  30, 225,   0,   0,   1,   1,   2,   2, 129, 128, 128,   0,   0, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0, 128,   0,  48,  48,   0,   0,   1,
        225,  26,   6,   9,  49,  53,   1, 138, 124,   0,   0, 128, 128, 128, 128,  64,
         64,  64,  64,  32,  32,  32,  32,  16,  16,  16,  16,   8,   8,   8,   8,   8,
          4,   4,   4,   4,   4,   2,   2,   2,   2,   1,   1,   1,   1,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        128, 112,  12,   3,   0,   0,  24,   6,   5, 152, 153, 132, 195, 124,  65,  65,
         64,  64,  32,  33,  34,  18,  17,  17,  17,   9,   8,   8,   8,   8,   4,   4,
          4,   4,   4,   4,   2,   2,   2,   1,   1,   1,   1,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128, 128, 128, 128,  64,  64,  64,  64,  32,  32,  32,  32,
         16,  16,  16,  16,  16,   8,   8,   8,   8,   8,   4,   4,   4,   4,   4,   2,
          3,   2,   2,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    },
    // [BONGO_TAP_L]
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128,   0,   0,   0,   0,   0, 128,  64,  64,  32,  32,  32,
         32,  16,  16,  16,  16,   8,   4,   2,   1,   1,   2,  12,  48,  64, 128,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 128, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,  30, 225,   0,   0,   1,   1,   2,   2, 129, 128, 128,   0,   0, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0, 128,   0,  48,  48,   0,   0,   1,
          1,   2,   4,   8,  16,  32,  64, 128,   0,   0,   0, 128, 128, 128, 128,  64,
         64,  64,  64,  32,  32,  32,  32,  16,  16,  16,  16,   8,   8,   8,   8,   8,
          4,   4,   4,   4,   4,   2,   2,   2,   2,   1,   1,   1,   1,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        128, 112,  12,   3,   0,   0,  24,   6,   5, 152, 153, 132,  67, 124,  65,  65,
         64,  64,  32,  33,  34,  18,  17,  17,  17,   9,   8,   8,   8,   8,   4,   4,
          8,   8,  16,  16,  16,  16,  16,  17,  15,   1,   1,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128, 128, 128, 128,  64,  64,  64,  64,  32,  32,  32,  32,
         32,  16,  16,  16,  16,   8,   8,   8,   8,   8,   4,   4,   4,   4,   4,   2,
          3,   2,   2,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    },
    // [BONGO_TAP_R]
    {
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128,   0,   0,   0,   0,   0, 128,  64,  64,  32,  32,  32,
         32,  16,  16,  16,  16,   8,   4,   2,   1,   1,   2,  12,  48,  64, 128,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 128, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,  30, 225,   0,   0,   1,   1,   2,   2,   1,   0,   0,   0,   0, 128, 128,
          0,   0,   0,   0,   0,   0,   0,   0,   0, 128,   0,  48,  48,   0,   0,   1,
        225,  26,   6,   9,  49,  53,   1, 138, 124,   0,   0, 128, 128, 128, 128,  64,
         64,  64,  64,  32,  32,  32,  32,  16,  16,  16,  16,   8,   8,   8,   8,   8,
          4,   4,   4,   4,   4,   2,   2,   2,   2,   1,   1,   1,   1,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        128, 112,  12,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
          0,  64, 160,  33,  34,  18,  17,  17,  17,   9,   8,   8,   8,   8,   4,   4,
          4,   4,   4,   4,   2,   2,   2,   1,   1,   1,   1,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0, 128, 128, 128, 128, 128,  64,  64,  64,  64,  32,  32,  32,  32,
         32,  16,  16,  16,  16,   8,   8,   8,   8,   8,   4,   4,   4,   4,   4,   2,
          3,   2,   2,   1,   1,   1,   1,   1,   1,   2,   2,   4,   4,   8,   8,   8,
          8,   8,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    },
};

// Zeichnet einen Frame auf die Surface. Statt 4096 Einzelpixel werden pro
// Zeile zusammenhaengende gesetzte Pixel als ein qp_rect ausgegeben - bei
// Strichgrafik sind das nur eine Handvoll Aufrufe pro Zeile.
static void bongo_draw(uint8_t frame) {
    const uint8_t *f = bongo_frames[frame];

    qp_rect(lcd_surface, BONGO_X, BONGO_Y, BONGO_X + BONGO_W - 1, BONGO_Y + BONGO_H - 1, HSV_BLACK, true);

    for (uint8_t y = 0; y < BONGO_H; y++) {
        const uint8_t *page = f + (y >> 3) * BONGO_W;
        const uint8_t  bit  = y & 7;

        uint16_t x = 0;
        while (x < BONGO_W) {
            if (!((page[x] >> bit) & 1)) {
                x++;
                continue;
            }
            uint16_t start = x;
            while (x < BONGO_W && ((page[x] >> bit) & 1)) {
                x++;
            }
            qp_rect(lcd_surface, BONGO_X + start, BONGO_Y + y, BONGO_X + x - 1, BONGO_Y + y, HSV_BONGO, true);
        }
    }
}

// ---------------------------------------------------------------------------
//  Anzeige
// ---------------------------------------------------------------------------
// Wird von halcyon.c am Anfang von display_module_housekeeping_task_kb()
// aufgerufen. Rueckgabe false => die splitkb-Standardausgabe wird komplett
// uebersprungen, wir zeichnen und flushen selbst.
//
// Drei Zustaende, gesteuert von last_input_activity_elapsed():
//   < HLC_IDLE_ANIM_START                         Ebenenname + Caps/Num/Scroll
//   < HLC_IDLE_ANIM_START + HLC_IDLE_ANIM_DURATION Bongo Cat
//   darueber                                      Panel und Beleuchtung sind
//                                                 aus, wir zeichnen nichts

bool display_module_housekeeping_task_user(bool second_display) {
    // Zweites Display (Slave-Haelfte, wenn der Master ebenfalls ein Display
    // hat): splitkb-Standard (Game of Life) weiterlaufen lassen.
    if (second_display) {
        return true;
    }

    static bool     initialised = false;
    static bool     idle_active = false;
    static bool     force       = true;
    static uint8_t  last_layer  = 0xFF;
    static led_t    last_locks  = {0};
    static uint32_t bongo_timer = 0;
    static uint8_t  bongo_frame = BONGO_TAP_L;

    if (!initialised) {
        hlc_font    = qp_load_font_mem(font_Retron2000_27);
        hlc_font_ul = qp_load_font_mem(font_Retron2000_underline_27);
        initialised = true;
    }
    if (hlc_font == NULL || hlc_font_ul == NULL) {
        return false; // Font nicht ladbar - nichts zeichnen, kb-Teil auslassen
    }

    const uint32_t idle = last_input_activity_elapsed();

    // ------------------------------------------------------------ Standby
    // Ab hier haben QMKs qp_internal_display_timeout_task() und halcyon.c
    // Panel und Hintergrundbeleuchtung abgeschaltet. Zeichnen waere sinnlos.
    //
    // idle_active bleibt hier bewusst stehen: das Panel behaelt die zuletzt
    // uebertragene Katze in seinem eigenen RAM. Beim naechsten Anschlag muss
    // deshalb der ganze Schirm geloescht werden, nicht nur die Textzeilen -
    // das erledigt der if(idle_active)-Block im Normalbetrieb weiter unten.
    if (idle >= (uint32_t)(HLC_IDLE_ANIM_START + HLC_IDLE_ANIM_DURATION)) {
        return false;
    }

    // ------------------------------------------------------ Bildschirmschoner
    if (idle >= (uint32_t)HLC_IDLE_ANIM_START) {
        if (!idle_active) {
            qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_BLACK, true);
            idle_active = true;
            force       = true;
            bongo_timer = timer_read32() - BONGO_FRAME_MS; // sofort zeichnen
        }

        if (timer_elapsed32(bongo_timer) >= BONGO_FRAME_MS) {
            bongo_timer = timer_read32();
            // Die beiden Tatzen abwechselnd - die Katze trommelt.
            // Fuer eine ruhende Katze stattdessen BONGO_WAITING zeichnen.
            bongo_frame = (bongo_frame == BONGO_TAP_L) ? BONGO_TAP_R : BONGO_TAP_L;
            bongo_draw(bongo_frame);
            qp_surface_draw(lcd_surface, lcd, 0, 0, 0);
            qp_flush(lcd);
        }
        return false;
    }

    // --------------------------------------------------------- Normalbetrieb
    if (idle_active) {
        // Gerade aufgewacht: Katze wegraeumen
        qp_rect(lcd_surface, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, HSV_BLACK, true);
        idle_active = false;
        force       = true;
    }

    bool dirty = force;
    force      = false;

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
