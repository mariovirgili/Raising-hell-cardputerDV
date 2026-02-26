#pragma once

#include <stdint.h>

struct InputState;

// Count/label are used by renderer; activate is used by input handler.
int uiSleepMenuCount();
const char* uiSleepMenuLabel(int idx);

// Run the action for the selected menu item.
// Returns true if an action ran.
bool uiSleepMenuActivate(int idx, InputState& in);