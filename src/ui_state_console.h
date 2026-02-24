#pragma once

struct InputState;
#include "ui_defs.h"

// Console UI state handler
void uiConsoleHandle(InputState& input);

// Opens the console and remembers where to return.
void openConsoleWithReturn(UIState returnState, Tab returnTab, bool retToSettings, SettingsPage retSettingsPage);