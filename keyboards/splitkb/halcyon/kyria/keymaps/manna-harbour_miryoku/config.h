// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

#pragma once

// ---------------------------------------------------------------------------
// Halcyon-Modul-Config
// ---------------------------------------------------------------------------
// WICHTIG: Encoder und TFT-Display belegen dieselben Pins (26 / 27 / 16) und
// halcyon.c erlaubt nur EIN "module_t module" pro Build. Deshalb darf immer
// nur genau eine Modul-Config eingebunden werden. Die Flags HLC_*_BUILD werden
// in der rules.mk aus den -e Variablen erzeugt.

#include "users/halcyon_modules/splitkb/config.h"

// ---------------------------------------------------------------------------
// Leerlauf: Bongo Cat, danach echter Standby
// ---------------------------------------------------------------------------
// Zwei unabhaengige Mechanismen schalten das Display ab, beide gemessen an
// last_input_activity_elapsed():
//   - QMKs qp_internal_display_timeout_task() ruft qp_power(false) auf,
//     sobald QUANTUM_PAINTER_DISPLAY_TIMEOUT erreicht ist
//   - halcyon.c schaltet bei HLC_BACKLIGHT_TIMEOUT die Beleuchtung ab
// hlc_tft_display/config.h setzt QUANTUM_PAINTER_DISPLAY_TIMEOUT auf
// HLC_BACKLIGHT_TIMEOUT. Es reicht daher, HLC_BACKLIGHT_TIMEOUT anzuheben -
// dann bleiben Panel und Beleuchtung waehrend der Animation an, und beide
// gehen anschliessend gemeinsam aus.
//
// Zeitachse ab der letzten Eingabe:
//   0 s   .. 120 s   Ebenenname + Caps/Num/Scroll
//   120 s .. 240 s   Bongo Cat
//   ab 240 s         Panel und Beleuchtung aus, bis wieder getippt wird

#define HLC_IDLE_ANIM_START 120000     // wann die Katze erscheint
#define HLC_IDLE_ANIM_DURATION 120000  // wie lange sie trommelt

#undef HLC_BACKLIGHT_TIMEOUT
#define HLC_BACKLIGHT_TIMEOUT (HLC_IDLE_ANIM_START + HLC_IDLE_ANIM_DURATION)

#ifdef HLC_ENCODER_BUILD
#    include "users/halcyon_modules/splitkb/hlc_encoder/config.h"
#endif

#ifdef HLC_TFT_DISPLAY_BUILD
#    include "users/halcyon_modules/splitkb/hlc_tft_display/config.h"
#endif

// ---------------------------------------------------------------------------
// Split-Transport
// ---------------------------------------------------------------------------
// Hinweis: SPLIT_LAYER_STATE_ENABLE, SPLIT_LED_STATE_ENABLE und
// SPLIT_MODS_ENABLE setzt users/halcyon_modules/splitkb/config.h bereits.
// Sie werden hier nicht noch einmal gesetzt.
// Beide Firmwares muessen dieselben SPLIT_*-Optionen haben, sonst passen die
// Transport-Datenstrukturen nicht zusammen - durch die gemeinsame config.h
// ist das gegeben.

#define SPLIT_TRANSPORT_MIRROR

// ---------------------------------------------------------------------------
// Legacy-Keycode-Aliase (Miryoku nutzt alte Namen, QMK hat sie umbenannt)
// ---------------------------------------------------------------------------
// Maustasten
#define KC_BTN1 MS_BTN1
#define KC_BTN2 MS_BTN2
#define KC_BTN3 MS_BTN3
// Mausbewegung
#define KC_MS_L MS_LEFT
#define KC_MS_D MS_DOWN
#define KC_MS_U MS_UP
#define KC_MS_R MS_RGHT
// Mausrad
#define KC_WH_L MS_WHLL
#define KC_WH_D MS_WHLD
#define KC_WH_U MS_WHLU
#define KC_WH_R MS_WHLR
// RGB Matrix (Miryoku nutzt alte RGB_*-Namen)
#define RGB_TOG RM_TOGG
#define RGB_MOD RM_NEXT
#define RGB_HUI RM_HUEU
#define RGB_SAI RM_SATU
#define RGB_VAI RM_VALU

// ---------------------------------------------------------------------------
// Miryoku Layout-Mapping fuer LAYOUT_split_3x6_5
// ---------------------------------------------------------------------------
#define XXX KC_NO

#define LAYOUT_miryoku( \
    K00, K01, K02, K03, K04,      K05, K06, K07, K08, K09, \
    K10, K11, K12, K13, K14,      K15, K16, K17, K18, K19, \
    K20, K21, K22, K23, K24,      K25, K26, K27, K28, K29, \
    N30, N31, K32, K33, K34,      K35, K36, K37, N38, N39  \
) \
LAYOUT_split_3x6_5( \
XXX, K00, K01, K02, K03, K04,           K05, K06, K07, K08, K09, XXX, \
XXX, K10, K11, K12, K13, K14,           K15, K16, K17, K18, K19, XXX, \
XXX, K20, K21, K22, K23, K24, XXX, XXX,  XXX, XXX, K25, K26, K27, K28, K29, XXX, \
               K32, K33, K34, XXX, XXX,  XXX, XXX, K35, K36, K37 \
)
