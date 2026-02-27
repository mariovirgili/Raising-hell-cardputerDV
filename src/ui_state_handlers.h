#pragma once

#include "ui_defs.h"

struct InputState;

using StateHandlerFn = void (*)(InputState&);

// Dispatches to the handler for the given UIState.
// Returns true if a handler existed and was called.
bool uiDispatchToStateHandler(UIState state, InputState& in);

// Optional: access the handler pointer (useful for diagnostics).
//
// IMPORTANT: We intentionally avoid indexing an array by enum value.
// UIState values have shifted over time (and may not be contiguous in older
// snapshots). A switch keeps the mapping correct regardless of enum ordering.
StateHandlerFn uiGetStateHandler(UIState state);
