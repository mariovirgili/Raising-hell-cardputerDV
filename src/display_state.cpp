#include "display_state.h"

// NOTE:
// This file used to *define* a bunch of global state (screenW/screenH, batteryPercent,
// usbPowered, brightnessLevel, etc.).
//
// That state has been split into dedicated modules:
//  - ui_layout_state.cpp        -> screenW, screenH
//  - system_status_state.cpp    -> batteryPercent, usbPowered
//  - brightness_state.cpp       -> brightnessLevel (and values)
//  - auto_screen.cpp            -> g_screenIsOff
//  - power_buttons_state.cpp    -> screenJustWentOff
//
// So this unit intentionally defines *nothing* now. It exists only as a compatibility shim.

#include "display_state.h"
