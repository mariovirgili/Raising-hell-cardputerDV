// ui_state_handlers.h
#pragma once

#include "ui_defs.h"

struct InputState;

// Dispatches input to the active UI state handler.
// Returns true if state existed and handler was called.
bool uiDispatchToStateHandler(UIState state, InputState& in);