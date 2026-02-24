#include "ui_state_handlers.h"

#include "ui_state_settings.h"
#include "ui_state_console.h"
#include "ui_state_wifi_setup.h"

bool uiDispatchToStateHandler(UIState st, InputState& in) {
  switch (st) {
    case UIState::SETTINGS:
      uiSettingsHandle(in);
      return true;

    case UIState::CONSOLE:
      uiConsoleHandle(in);
      return true;

    case UIState::WIFI_SETUP:
      uiWifiSetupHandle(in);
      return true;

    default:
      return false;
  }
}