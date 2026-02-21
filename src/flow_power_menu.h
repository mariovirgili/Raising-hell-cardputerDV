// flow_power_menu.h
#pragma once

#include <stdint.h>

struct InputState;

// Enter power menu (snapshots return target, wakes screen if needed)
void openPowerMenuFromHere(uint32_t nowMs);

// Power menu input handler (called from menu_actions dispatcher)
void handlePowerMenuInput(InputState& input);

// Used by sleep-screen input to suppress instant wake after closing Power Menu
bool powerMenuSleepWakeSuppressedNow();
