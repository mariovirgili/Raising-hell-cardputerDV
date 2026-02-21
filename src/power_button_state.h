#pragma once
#include <stdbool.h>
#include <stdint.h>

// Power / screen button transient state
extern bool     screenButtonHeld;
extern uint32_t screenButtonPressTime;
extern bool     screenJustWentOff;

// UI power menu flag (if still used outside AppState)
extern bool     inPowerMenu;
