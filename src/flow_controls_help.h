#pragma once

struct InputState;

// Public entry points used by app hotkeys / settings menu
void openControlsHelpFromSettings();
void openControlsHelpFromAnywhere();

// Controls help UIState handler (routed by ui_input_router)
void uiControlsHelpHandle(InputState& in);