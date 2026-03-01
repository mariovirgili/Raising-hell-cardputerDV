#pragma once

struct InputState;

int uiPlayMenuCount();
const char* uiPlayMenuLabel(int idx);

// Returns true if it ran an action
bool uiPlayMenuActivate(int idx, InputState& in);