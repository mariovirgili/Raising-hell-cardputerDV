// flow_power_menu.h
#pragma once

#include <stdint.h>

struct InputState;

// Power menu input handler (called from menu_actions dispatcher)
void handlePowerMenuInput(InputState& input);

// Power menu
void openPowerMenuFromHere(uint32_t now);