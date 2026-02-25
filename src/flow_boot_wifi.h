// flow_boot_wifi.h
#pragma once

#include "ui_defs.h"

struct InputState;

// Boot wizard entry point (called by boot_pipeline)
void bootWizardBegin(UIState afterState, Tab afterTab);

// Handlers called by menu_actions dispatcher
void handleBootWifiPromptInput(InputState& in);
void handleBootWifiWaitInput(InputState& in);
void handleBootTzPickInput(InputState& in);
void handleBootNtpWaitInput(InputState& in);

void uiBootWifiPromptHandle(InputState& in);
void uiBootWifiWaitHandle(InputState& in);
void uiBootTzPickHandle(InputState& in);
void uiBootNtpWaitHandle(InputState& in);