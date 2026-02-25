#pragma once
struct InputState;

// Returns true if input was consumed (routing should stop)
bool uiHandleGlobalInterceptors(InputState& in);