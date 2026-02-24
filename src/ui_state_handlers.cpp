#include "ui_state_handlers.h"

#include "ui_state_settings.h"
#include "ui_state_console.h"

// If you have a migrated wifi-setup handler, include it here.
// If not, do NOT include/dispatch it yet.
// #include "ui_state_wifi_setup.h"

bool uiDispatchToStateHandler(UIState st, InputState& in) {
  switch (st) {
    case UIState::SETTINGS:
      uiSettingsHandle(in);
      return true;

    case UIState::CONSOLE:
      uiConsoleHandle(in);
      return true;

    // Uncomment only if you have uiWifiSetupHandle migrated + compiling.
    // case UIState::WIFI_SETUP:
    //   uiWifiSetupHandle(in);
    //   return true;

    default:
      return false;
  }
}