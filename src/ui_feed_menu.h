#pragma once

struct InputState;

int uiFeedMenuCount();
const char* uiFeedMenuLabel(int idx);

// Returns true if it ran an action
bool uiFeedMenuActivate(int idx, InputState& in);