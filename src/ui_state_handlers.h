#pragma once

#include "ui_defs.h"
#include "ui_state_settings.h"

struct InputState;

// Dispatches input to the active UI state handler.
// Returns true if state existed and handler was called.
bool uiDispatchToStateHandler(UIState state, InputState& in);