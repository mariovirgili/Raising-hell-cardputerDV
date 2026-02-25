#pragma once

struct InputState;

// Thin entry point used by app_loop.cpp (or wherever you call it)
bool handleMenuInput(InputState& in);

// Called by mini_games.cpp when the resurrection game ends
void onResurrectionMiniGameResult(bool success);

// Called by settings actions
void settingsToggleLedAlerts();

extern int deathMenuIndex;

void resetDeathMenu();