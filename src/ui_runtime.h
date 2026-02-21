// ui_runtime.h
#pragma once
#include <stdint.h>
#include "ui_defs.h"        // UIState / Tab / SettingsPage enums

// NOTE:
// This struct is for *misc runtime UI indices / buffers* that are still being
// migrated into dedicated state modules. Settings navigation/return flow has been
// moved to settings_flow_state.* (g_settingsFlow).
struct UIRuntimeState {
  int screenSettingsIndex = 0;
  int systemSettingsIndex = 0;
  int gameOptionsIndex    = 0;
  int playIndex = 0;

  int autoScreenIndex = 0;
  int decayModeIndex  = 0;
};

extern UIRuntimeState g_ui;

void requestFullUIRedraw();
