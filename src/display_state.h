// display_state.h — Raising Hell (Cardputer ADV)
// Convenience umbrella header for display/system state.
// IMPORTANT: No storage lives here. Each state variable is declared in its
// owning *_state.h and defined exactly once in its *_state.cpp.
#pragma once

#include <stdint.h>

// Layout / dimensions
#include "display_dims_state.h"      // screenW, screenH

// Power/battery status
#include "system_status_state.h"     // batteryPercent, usbPowered, bootTime

// Backlight brightness
#include "brightness_state.h"        // brightnessLevel, brightnessValues, applyBrightnessLevel(...)

// Screen off pipeline

// Button edges for screen/power
#include "power_button_state.h"      // screenJustWentOff, etc.
