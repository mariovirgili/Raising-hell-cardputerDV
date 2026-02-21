#include "ui_state_utils.h"

bool isSettingsState(UIState s) {
  return (s == UIState::SETTINGS ||
          s == UIState::CONSOLE ||
          s == UIState::SET_TIME ||
          s == UIState::WIFI_SETUP);
}
