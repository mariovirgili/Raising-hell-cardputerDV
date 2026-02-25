#pragma once

#include "ui_defs.h"

struct InputState;

using StateHandlerFn = void(*)(InputState&);

// Dispatches input to the active UI state handler.
// Returns true if a handler existed and was called.
bool uiDispatchToStateHandler(UIState state, InputState& in);