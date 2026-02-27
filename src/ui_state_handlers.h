#pragma once

#include "ui_defs.h"

struct InputState;

using StateHandlerFn = void (*)(InputState&);

bool uiDispatchToStateHandler(UIState state, InputState& in);