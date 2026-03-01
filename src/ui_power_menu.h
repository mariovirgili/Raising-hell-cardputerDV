#pragma once

struct InputState;

// Catalog info for UI/rendering
int uiPowerMenuCount();
const char* uiPowerMenuLabel(int idx);

// Returns true if action ran
bool uiPowerMenuActivate(int idx, InputState& in);