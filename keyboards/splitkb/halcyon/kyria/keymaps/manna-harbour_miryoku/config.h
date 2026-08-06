// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

#pragma once

// Halcyon-Modul-Config direkt einbinden
#include "users/halcyon_modules/splitkb/config.h"
#include "users/halcyon_modules/splitkb/hlc_encoder/config.h"

#define SPLIT_TRANSPORT_MIRROR


// --- Legacy-Keycode-Aliase (Miryoku nutzt alte Namen, QMK hat sie umbenannt) ---
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

// --- Miryoku Layout-Mapping für LAYOUT_split_3x6_5 ---
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
