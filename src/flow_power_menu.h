// flow_power_menu.h
#pragma once

#include <stdint.h>

struct InputState;

// Power menu interceptors (called from ui_input_interceptors)
void openPowerMenuFromHere(uint32_t now);

void uiPowerMenuHandle(InputState& in);