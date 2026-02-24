#pragma once

#include "ui_defs.h"

struct InputState;

// Returns true if the state was handled by a dedicated ui_state_* handler.
// Returns false to allow legacy handling elsewhere.
bool uiDispatchToStateHandler(UIState state, InputState& in);