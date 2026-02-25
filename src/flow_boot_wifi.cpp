// flow_boot_wifi.cpp
#include "flow_boot_wifi.h"

#include <Arduino.h>
#include <time.h>

#include "app_state.h"
#include "input.h"
#include "ui_runtime.h"
#include "display.h"
#include "wifi_setup_state.h"
#include "wifi_time.h"
#include "timezone.h"
#include "sound.h"
#include "save_manager.h"
#include "time_persist.h"
#include "time_state.h"
#include "graphics.h"
#include "new_pet_flow_state.h"
#include "menu_actions.h"
#include "flow_boot_wizard.h"
#include "ui_settings_actions.h"

// Wizard state (moved out of menu_actions.cpp)
static bool    s_bootWifiWizardActive = false;
static UIState s_afterState = UIState::CHOOSE_PET;
static Tab     s_afterTab   = Tab::TAB_PET;

// Small helper to drain queued key events safely
static inline void drainKb(InputState& in) {
  while (in.kbHasEvent()) (void)in.kbPop();
}

// “Synced enough” for our purposes: valid epoch beyond a floor.
static inline bool timeSyncedNow() {
  time_t now = time(nullptr);
  return (now > 1700000000); // ~late 2023
}

static void bootWizardSkipToManualTime() {
  s_bootWifiWizardActive    = false;
  g_wifiSetupFromBootWizard = false;
  beginForcedSetTimeBootGate(s_afterState, s_afterTab);

  g_app.uiState    = UIState::SET_TIME;
  g_app.currentTab = s_afterTab;

  requestFullUIRedraw();
  clearInputLatch();
}

void bootWizardBegin(UIState afterState, Tab afterTab) {
  s_bootWifiWizardActive = true;
  s_afterState = afterState;
  s_afterTab   = afterTab;

  g_app.uiState    = UIState::BOOT_WIFI_PROMPT;
  g_app.currentTab = Tab::TAB_PET;

  requestFullUIRedraw();
  clearInputLatch();
  }

void handleBootWifiPromptInput(InputState& in) {
  if (in.escOnce || in.menuOnce) {
    bootWizardSkipToManualTime();
    return;
  }

  if (in.selectOnce || in.encoderPressOnce) {
    g_wifiSetupFromBootWizard = true;
    g_wifi.setupStage = 0;
    g_wifi.buf[0]     = '\0';
    g_wifi.ssid[0]    = '\0';
    g_wifi.pass[0]    = '\0';

    g_app.uiState = UIState::WIFI_SETUP;
    requestUIRedraw();

    inputSetTextCapture(true);
    g_textCaptureMode = true;

    drainKb(in);
    clearInputLatch();
    playBeep();
    return;
  }

  drainKb(in);
  clearInputLatch();
}

void handleBootWifiWaitInput(InputState& in) {
  if (in.escOnce || in.menuOnce) {
    bootWizardSkipToManualTime();
    return;
  }

  if (wifiIsConnectedNow()) {
    g_app.uiState = UIState::BOOT_TZ_PICK;
    requestFullUIRedraw();
    clearInputLatch();
    return;
  }

  drainKb(in);
  clearInputLatch();
}

void handleBootTzPickInput(InputState& in) {
  if (in.escOnce || in.menuOnce) {
    g_app.uiState = UIState::BOOT_NTP_WAIT;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  int mv = in.encoderDelta;
  if (in.upOnce) mv = -1;
  if (in.downOnce) mv = 1;

  if (mv != 0) {
    settingsCycleTimeZone(mv < 0 ? -1 : +1);
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  if (in.selectOnce || in.encoderPressOnce) {
    saveSettingsToSD();
    saveTzIndexToNVS((uint8_t)tzIndex);
    applyTimezoneIndex((uint8_t)tzIndex);

    g_app.uiState = UIState::BOOT_NTP_WAIT;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  drainKb(in);
  clearInputLatch();
}

void handleBootNtpWaitInput(InputState& in) {
  if (in.escOnce || in.menuOnce) {
    bootWizardSkipToManualTime();
    return;
  }

  if (wifiIsConnectedNow() && timeSyncedNow() && timeIsValid()) {
    saveTimeAnchor();
    updateTime();

    s_bootWifiWizardActive    = false;
    g_wifiSetupFromBootWizard = false;
    
    g_app.uiState    = s_afterState;
    g_app.currentTab = s_afterTab;

    if (g_app.uiState == UIState::CHOOSE_PET) {
      // This symbol lives in menu_actions.cpp currently.
    }

   requestFullUIRedraw();
   clearInputLatch();
    return;
  }

  drainKb(in);
  clearInputLatch();
}
