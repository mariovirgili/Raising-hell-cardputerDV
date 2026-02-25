// flow_controls_help.h
#pragma once

struct InputState;

// Public entry points used by app hotkeys / settings menu
void openControlsHelpFromSettings();
void openControlsHelpFromAnywhere();

// Handler called by menu_actions dispatcher
void handleControlsHelpInput(InputState& in);

void uiControlsHelpHandle(InputState& in);