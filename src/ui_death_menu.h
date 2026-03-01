#pragma once

struct InputState;

int uiDeathMenuCount();
const char* uiDeathMenuLabel(int idx);

// Returns true if action ran
bool uiDeathMenuActivate(int idx, InputState& in);