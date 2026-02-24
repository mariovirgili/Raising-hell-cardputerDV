#include "ui_state_settings.h"

// Needs the full InputState definition
#include "input.h"

// App + settings flow globals/types
#include "app_state.h"
#include "settings_flow_state.h"
#include "ui_defs.h"

// UI + audio
#include "ui_runtime.h"
#include "sound.h"

// Saving
#include "sdcard.h"
#include "save_manager.h"

// Screen/brightness/auto screen
#include "display.h"
#include "auto_screen.h"
#include "brightness_state.h"

// Console + flows invoked from settings
#include "console.h"
#include "flow_controls_help.h"
#include "flow_factory_reset.h"
#include "flow_time_editor.h"

// WiFi controls used from settings
#include "wifi_power.h"
#include "wifi_store.h"
#include "wifi_setup_state.h"

// Misc toggles used in settings pages
#include "led_status.h"
#include "game_options_state.h"

#include <stdint.h>

#include "settings_nav_state.h"
#include "time_persist.h"
#include "ui_input_utils.h"
#include "menu_actions.h"   // openConsoleWithReturn, settingsCycleTimeZone, (we’ll add resetSettingsNav to this header)
#include "wifi_time.h"      // wifiIsEnabled, wifiSetEnabled
#include "graphics.h"       // ui_showMessage
#include "ui_input_utils.h" // uiDrainKb (new shared helper)
#include "ui_settings_pages.h"
#include "ui_input_utils.h"

void resetSettingsNav(bool resetTopIndex);

void uiSettingsHandle(InputState& input) {
    // ESC or MENU exits settings OR backs out of picker subpages
    if (input.menuOnce || input.escOnce) {
      // If inside picker pages, go back one level
      if (g_settingsFlow.settingsPage == SettingsPage::DECAY_MODE ||
          g_settingsFlow.settingsPage == SettingsPage::AUTO_SCREEN) {
        g_settingsFlow.settingsPage = g_settingsFlow.settingsReturnPage;
        requestUIRedraw();
        playBeep();
        clearInputLatch();
        return;
      }
  
      // Otherwise exit settings completely
      resetSettingsNav(true);
  
      UIState retState = UIState::PET_SCREEN;
      Tab     retTab   = Tab::TAB_PET;
  
      if (g_settingsFlow.settingsReturnValid) {
        retState = g_settingsFlow.settingsReturnState;
        retTab   = g_settingsFlow.settingsReturnTab;
      }
  
      g_settingsFlow.settingsReturnValid = false;
  
      g_app.uiState    = retState;
      g_app.currentTab = retTab;
      requestUIRedraw();
  
      playBeep();
      clearInputLatch();
      return;
    }
  
    int move = input.encoderDelta;
    if (input.upOnce) move = -1;
    if (input.downOnce) move = 1;
  
    switch (g_settingsFlow.settingsPage) {
      case SettingsPage::TOP:        UiSettingsPages::Handle_TOP(input, move);        return;
      case SettingsPage::SCREEN:     UiSettingsPages::Handle_SCREEN(input, move);     return;
      case SettingsPage::SYSTEM:     UiSettingsPages::Handle_SYSTEM(input, move);     return;
      case SettingsPage::WIFI:       UiSettingsPages::Handle_WIFI(input, move);       return;
      case SettingsPage::GAME:       UiSettingsPages::Handle_GAME(input, move);       return;
      case SettingsPage::AUTO_SCREEN:UiSettingsPages::Handle_AUTO_SCREEN(input, move);return;
      case SettingsPage::DECAY_MODE: UiSettingsPages::Handle_DECAY_MODE(input, move); return;
      default:
        return;
    }
  }
