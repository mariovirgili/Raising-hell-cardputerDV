#pragma once

struct InputState;

// Central UI input entry point.
// Applies global interceptors (boot fixups, suppression windows, sleep gate, etc.)
// then dispatches to the current UIState handler table.
bool uiHandleInput(InputState& in);