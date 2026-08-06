CFLAGS += -Wno-error=sizeof-pointer-div

# --- Halcyon-Module: vollständige splitkb-Konfiguration ---
HALCYON_PATH := users/halcyon_modules

# Features, die halcyon.c benötigt
POINTING_DEVICE_ENABLE = yes
POINTING_DEVICE_DRIVER = cirque_pinnacle_spi
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi surface
BACKLIGHT_ENABLE = yes
BACKLIGHT_DRIVER = pwm

VPATH += $(HALCYON_PATH)/splitkb/
SRC += $(HALCYON_PATH)/splitkb/halcyon.c
HALCONFDIR += $(HALCYON_PATH)/splitkb/halconf.h
POST_CONFIG_H += $(HALCYON_PATH)/splitkb/config.h

ifeq ($(filter 1, $(HLC_ENCODER) $(HLC_ENCODER_REV2)), 1)
  include $(HALCYON_PATH)/splitkb/hlc_encoder/rules.mk
  SRC += $(HALCYON_PATH)/splitkb/halcyon_buttons.c
endif

ifdef HLC_TFT_DISPLAY
  include $(HALCYON_PATH)/splitkb/hlc_tft_display/rules.mk
endif

ifdef HLC_CIRQUE_TRACKPAD
  include $(HALCYON_PATH)/splitkb/hlc_cirque_trackpad/rules.mk
endif

HLC_OPTIONS := $(HLC_NONE) $(HLC_CIRQUE_TRACKPAD) $(HLC_ENCODER) $(HLC_TFT_DISPLAY) $(HLC_ENCODER_REV2)
ifeq ($(filter 1, $(HLC_OPTIONS)), )
$(error Halcyon module used but none specified. Add -e HLC_ENCODER=1)
endif
