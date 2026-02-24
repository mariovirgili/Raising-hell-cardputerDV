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

void resetSettingsNav(bool resetTopIndex);

// We keep the implementation in this file.
// This is the old handleSettingsInput() body moved over verbatim.

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

  // ---- TOP page ----
  if (g_settingsFlow.settingsPage == SettingsPage::TOP) {
    if (move != 0) {
      const int totalItems = 7;
      g_app.settingsIndex += move;
      if (g_app.settingsIndex < 0) g_app.settingsIndex = totalItems - 1;
      if (g_app.settingsIndex > totalItems - 1) g_app.settingsIndex = 0;

      requestUIRedraw();
      playBeep();
      return;
    }

    if (g_app.settingsIndex == 0) {
      if (input.leftOnce || input.rightOnce) {
        soundAdjustVolume(input.leftOnce ? -1 : 1);
        saveSettingsToSD();
        saveManagerMarkDirty();
        requestUIRedraw();
        playBeep();
        clearInputLatch();
        return;
      }
    }

    if (input.selectOnce || input.encoderPressOnce) {
      switch (g_app.settingsIndex) {
        case 0: { // Volume cycles
          soundAdjustVolume(+1);
          saveSettingsToSD();
          saveManagerMarkDirty();
          requestUIRedraw();
          playBeep();
          clearInputLatch();
          return;
        }

        case 1: { // Controls
          openControlsHelpFromSettings();
          playBeep();
          clearInputLatch();
          return;
        }

        case 2: { // Screen Settings
          g_settingsFlow.settingsPage = SettingsPage::SCREEN;
          g_app.screenSettingsIndex    = 0;
          requestUIRedraw();
          playBeep();
          clearInputLatch();
          return;
        }

        case 3: { // System
          g_settingsFlow.settingsPage = SettingsPage::SYSTEM;
          g_app.systemSettingsIndex    = 0;
          requestUIRedraw();
          playBeep();
          clearInputLatch();
          return;
        }

        case 4: { // Game
          g_settingsFlow.settingsPage = SettingsPage::GAME;
          g_app.gameOptionsIndex       = 0;
          requestUIRedraw();
          playBeep();
          clearInputLatch();
          return;
        }

        case 5: { // Console
#if PUBLIC_BUILD
          ui_showMessage("Console disabled");
          soundError();
          clearInputLatch();
          return;
#else
          openConsoleWithReturn(UIState::SETTINGS, g_app.currentTab, true, g_settingsFlow.settingsPage);
          uiDrainKb(input);
          clearInputLatch();
          requestUIRedraw();
          playBeep();
          return;
#endif
        }

        case 6: { // Credits
          g_settingsFlow.settingsPage = SettingsPage::CREDITS;
          requestUIRedraw();
          playBeep();
          clearInputLatch();
          return;
        }

        default: break;
      }
    }

    return; // IMPORTANT: stop here for TOP page
  }

  // ---- SCREEN page ----
  if (g_settingsFlow.settingsPage == SettingsPage::SCREEN) {
    if (move != 0) {
      const int totalItems = 2;
      g_app.screenSettingsIndex += move;
      if (g_app.screenSettingsIndex < 0) g_app.screenSettingsIndex = totalItems - 1;
      if (g_app.screenSettingsIndex > totalItems - 1) g_app.screenSettingsIndex = 0;

      requestUIRedraw();
      playBeep();
      return;
    }

    const bool leftPulse  = input.leftOnce;
    const bool rightPulse = input.rightOnce;

    if (g_app.screenSettingsIndex == 0 && (leftPulse || rightPulse)) {
      brightnessLevel += (rightPulse ? 1 : -1);
      if (brightnessLevel < 0) brightnessLevel = 2;
      if (brightnessLevel > 2) brightnessLevel = 0;

      setBacklight((uint16_t)brightnessValues[brightnessLevel]);

      saveSettingsToSD();
      saveManagerMarkDirty();
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (g_app.screenSettingsIndex == 1 && (input.selectOnce || input.encoderPressOnce)) {
      g_app.autoScreenIndex             = (int)autoScreenTimeoutSel;
      g_settingsFlow.settingsReturnPage = SettingsPage::SCREEN;
      g_settingsFlow.settingsPage       = SettingsPage::AUTO_SCREEN;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    return;
  }

  // ---- SYSTEM page ----
  if (g_settingsFlow.settingsPage == SettingsPage::SYSTEM) {
    if (move != 0) {
      const int totalItems = 3;
      g_app.systemSettingsIndex += move;
      if (g_app.systemSettingsIndex < 0) g_app.systemSettingsIndex = totalItems - 1;
      if (g_app.systemSettingsIndex > totalItems - 1) g_app.systemSettingsIndex = 0;

      requestUIRedraw();
      playBeep();
      return;
    }

    // Factory Reset confirm/hold UI is owned by flow_factory_reset.cpp now.
    if (factoryResetSystemSettingsHook(input, g_app.systemSettingsIndex)) {
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      switch (g_app.systemSettingsIndex) {
        case 0:
          beginSetTimeEditorFromSettings(SettingsPage::SYSTEM, UIState::SETTINGS, g_app.currentTab);          playBeep();
          return;

        case 2:
          g_settingsFlow.settingsPage = SettingsPage::WIFI;
          g_wifi.wifiSettingsIndex      = 0;
          requestUIRedraw();
          playBeep();
          clearInputLatch();
          return;

        default: break;
      }
    }
    return;
  }

  // ---- WIFI page ----
  if (g_settingsFlow.settingsPage == SettingsPage::WIFI) {
    const int totalItems = 4;

    if (move != 0) {
      g_wifi.wifiSettingsIndex += move;
      if (g_wifi.wifiSettingsIndex < 0) g_wifi.wifiSettingsIndex = totalItems - 1;
      if (g_wifi.wifiSettingsIndex > totalItems - 1) g_wifi.wifiSettingsIndex = 0;

      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    // Timezone cycling on the TZ row (row 3)
    if (g_wifi.wifiSettingsIndex == 3 && (input.leftOnce || input.rightOnce)) {
      settingsCycleTimeZone(input.leftOnce ? -1 : 1);
      // settingsCycleTimeZone() already redraws + beeps + clears latch
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      switch (g_wifi.wifiSettingsIndex) {
        case 0: { // WiFi ON/OFF toggle
          const bool en = !wifiIsEnabled();

          wifiSetEnabled(en);
          settingsSetWifiEnabled(en);
          applyWifiPower(en);

          saveSettingsToSD();
          saveManagerMarkDirty();

          requestUIRedraw();
          playBeep();
          clearInputLatch();
          return;
        }

        case 1: { // Set WiFi Network (SSID/PASS entry)
          g_wifiSetupFromBootWizard = false;

          g_wifi.setupStage = 0;
          g_wifi.buf[0]     = '\0';
          g_wifi.ssid[0]    = '\0';
          g_wifi.pass[0]    = '\0';

          g_app.uiState = UIState::WIFI_SETUP;
          requestUIRedraw();

          inputSetTextCapture(true);
          g_textCaptureMode = true;

          uiDrainKb(input);
          clearInputLatch();
          playBeep();
          return;
        }

        case 2: { // Reset WiFi Settings
          wifiResetSettings();
          wifiStoreClear();

          ui_showMessage("WiFi reset");
          requestUIRedraw();

          playBeep();
          clearInputLatch();
          return;
        }

        case 3: { // Time zone row: cycle forward on select
          settingsCycleTimeZone(+1);
          // settingsCycleTimeZone() already redraws + beeps + clears latch
          return;
        }

        default:
          break;
      }
    }

    return;
  }
  
  // ---- GAME OPTIONS page ----
  if (g_settingsFlow.settingsPage == SettingsPage::GAME) {
    const int totalItems = 3;

    int mv = input.encoderDelta;
    if (input.upOnce) mv = -1;
    if (input.downOnce) mv = 1;

    if (mv != 0) {
      g_app.gameOptionsIndex += mv;
      if (g_app.gameOptionsIndex < 0) g_app.gameOptionsIndex = totalItems - 1;
      if (g_app.gameOptionsIndex >= totalItems) g_app.gameOptionsIndex = 0;

      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      if (g_app.gameOptionsIndex == 0) {
        g_app.decayModeIndex              = (int)saveManagerGetDecayMode();
        g_settingsFlow.settingsReturnPage = SettingsPage::GAME;
        g_settingsFlow.settingsPage       = SettingsPage::DECAY_MODE;
        requestUIRedraw();
        playBeep();
        clearInputLatch();
        return;
      }

      if (g_app.gameOptionsIndex == 1) {
        petDeathEnabled = !petDeathEnabled;
        saveSettingsToSD();
        saveManagerMarkDirty();
        requestUIRedraw();
        playBeep();
        clearInputLatch();
        return;
      }

      if (g_app.gameOptionsIndex == 2) {
        ledAlertsEnabled = !ledAlertsEnabled;
#if LED_STATUS_ENABLED
        ledUpdatePetStatus(LED_PET_OFF);
#endif
        saveSettingsToSD();
        saveManagerMarkDirty();
        requestUIRedraw();
        playBeep();
        clearInputLatch();
        return;
      }
    }
    return;
  }

  // ---- AUTO SCREEN picker page ----
  if (g_settingsFlow.settingsPage == SettingsPage::AUTO_SCREEN) {
    const int kCount = 4;

    int mv = input.encoderDelta;
    if (input.upOnce) mv = -1;
    if (input.downOnce) mv = 1;

    if (mv != 0) {
      g_app.autoScreenIndex =
        (g_app.autoScreenIndex + (mv < 0 ? (kCount - 1) : 1)) % kCount;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      autoScreenTimeoutSel = (uint8_t)g_app.autoScreenIndex;
      autoScreenSetEnabled(autoScreenTimeoutSel != 0);
      screenWake();
      saveSettingsToSD();
      saveManagerMarkDirty();
      g_settingsFlow.settingsPage = g_settingsFlow.settingsReturnPage;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.menuOnce || input.escOnce) {
      g_settingsFlow.settingsPage = g_settingsFlow.settingsReturnPage;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }
    return;
  }

  // ---- DECAY MODE picker page ----
  if (g_settingsFlow.settingsPage == SettingsPage::DECAY_MODE) {
    const int kCount = 6;

    int mv = input.encoderDelta;
    if (input.upOnce) mv = -1;
    if (input.downOnce) mv = 1;

    if (mv != 0) {
      g_app.decayModeIndex =
        (g_app.decayModeIndex + (mv < 0 ? (kCount - 1) : 1)) % kCount;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      saveManagerSetDecayMode((uint8_t)g_app.decayModeIndex);
      g_settingsFlow.settingsPage = g_settingsFlow.settingsReturnPage;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.menuOnce || input.escOnce) {
      g_settingsFlow.settingsPage = g_settingsFlow.settingsReturnPage;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }
    return;
  }
}
