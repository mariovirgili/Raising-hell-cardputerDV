#pragma once
struct InputState;

int uiMgPauseMenuCount();
const char* uiMgPauseMenuLabel(int idx);
bool uiMgPauseMenuActivate(int idx, InputState& in);