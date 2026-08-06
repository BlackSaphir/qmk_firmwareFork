CFLAGS += -Wno-error=sizeof-pointer-div

# --- Halcyon-Module: vollstaendige splitkb-Konfiguration ---
HALCYON_PATH := users/halcyon_modules

# Features, die halcyon.c benoetigt
POINTING_DEVICE_ENABLE = yes
POINTING_DEVICE_DRIVER = cirque_pinnacle_spi
BACKLIGHT_ENABLE = yes
BACKLIGHT_DRIVER = pwm

VPATH   += $(HALCYON_PATH)/splitkb/
SRC     += $(HALCYON_PATH)/splitkb/halcyon.c
HALCONFDIR   += $(HALCYON_PATH)/splitkb/halconf.h
POST_CONFIG_H += $(HALCYON_PATH)/splitkb/config.h

# ---------------------------------------------------------------------------
# WICHTIG: die rules.mk der Halcyon-Module NICHT per include einbinden.
# Sie referenzieren ihre Dateien ueber $(USER_PATH), und QMK setzt USER_PATH
# erst NACH dieser Datei auf users/$(KEYMAP) -- also auf
# users/manna-harbour_miryoku statt auf users/halcyon_modules. Ergebnis waeren
# Linker-Fehler ("cannot find .../users/manna-harbour_miryoku/splitkb/...").
# Deshalb stehen die Quelldateien hier explizit mit $(HALCYON_PATH).
# ---------------------------------------------------------------------------

HLC_SRC_PATH := $(HALCYON_PATH)/splitkb

# ---------------------------------------------------------------------------
# Encoder-Modul
# ---------------------------------------------------------------------------
ifeq ($(filter 1, $(HLC_ENCODER) $(HLC_ENCODER_REV2)), 1)
  SRC += $(HLC_SRC_PATH)/halcyon_buttons.c

  POST_CONFIG_H += $(HLC_SRC_PATH)/hlc_encoder/config.h
  ifeq ($(HLC_ENCODER_REV2), 1)
    POST_CONFIG_H += $(HLC_SRC_PATH)/hlc_encoder/config_rev2.h
  endif

  # Flag fuer die config.h (POST_CONFIG_H kommt zu spaet fuer #ifdef dort)
  OPT_DEFS += -DHLC_ENCODER_BUILD
endif

# ---------------------------------------------------------------------------
# TFT-Display-Modul (Quantum Painter / ST7789)
# ---------------------------------------------------------------------------
ifeq ($(HLC_TFT_DISPLAY), 1)
  QUANTUM_PAINTER_ENABLE = yes
  QUANTUM_PAINTER_DRIVERS += st7789_spi surface

  HLC_TFT_PATH := $(HLC_SRC_PATH)/hlc_tft_display

  SRC += $(HLC_TFT_PATH)/hlc_tft_display.c

  # Fonts
  SRC += $(HLC_TFT_PATH)/graphics/fonts/Retron2000-27.qff.c \
         $(HLC_TFT_PATH)/graphics/fonts/Retron2000-underline-27.qff.c

  # Ziffern-Grafiken: werden von update_display() in hlc_tft_display.c
  # referenziert und muessen daher mitgelinkt werden, auch wenn unsere
  # keymap.c die Ebene als Text zeichnet.
  SRC += $(foreach n,0 1 2 3 4 5 6 7 8 9,$(HLC_TFT_PATH)/graphics/numbers/$(n).qgf.c) \
         $(HLC_TFT_PATH)/graphics/numbers/undef.qgf.c

  POST_CONFIG_H += $(HLC_TFT_PATH)/config.h

  # Damit die keymap.c "hlc_tft_display.h" und die Font-Header findet
  # (QMK haengt jedes VPATH-Verzeichnis auch an EXTRAINCDIRS an).
  VPATH += $(HLC_TFT_PATH)

  OPT_DEFS += -DHLC_TFT_DISPLAY_BUILD
endif

# ---------------------------------------------------------------------------
# Cirque-Trackpad-Modul
# ---------------------------------------------------------------------------
ifeq ($(HLC_CIRQUE_TRACKPAD), 1)
  POST_CONFIG_H += $(HLC_SRC_PATH)/hlc_cirque_trackpad/config.h
  OPT_DEFS += -DHLC_CIRQUE_TRACKPAD_BUILD
endif

# ---------------------------------------------------------------------------
# Plausibilitaetspruefung
# ---------------------------------------------------------------------------
HLC_OPTIONS := $(HLC_NONE) $(HLC_CIRQUE_TRACKPAD) $(HLC_ENCODER) $(HLC_TFT_DISPLAY) $(HLC_ENCODER_REV2)

ifeq ($(filter 1, $(HLC_OPTIONS)), )
$(error Halcyon module used but none specified. Add -e HLC_ENCODER=1 oder -e HLC_TFT_DISPLAY=1)
endif

# Genau ein Modul pro Build - Encoder und Display teilen sich die Pins 26/27/16
ifneq ($(words $(filter 1, $(HLC_OPTIONS))), 1)
$(error Nur genau ein Halcyon-Modul pro Build erlaubt. Baue je Haelfte eine eigene Firmware.)
endif
