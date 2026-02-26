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
#include "flow_boot_wizard.h"
#include "ui_settings_actions.h"
#include "ui_actions.h"
#include "flow_time_editor.h"

// Wizard state (moved out of menu_actions.cpp)
static bool    s_bootWifiWizardActive = false;
static UIState s_afterState = UIState::CHOOSE_PET;
static Tab     s_afterTab   = Tab::TAB_PET;

// Small helper to drain queued key events safely
static inline void drainKb(InputState& in)
{
  while (in.kbHasEvent()) (void)in.kbPop();
}

static inline void gotoState(UIState s, Tab t, bool fullRedraw)
{
  uiActionEnterState(s, t, true);
  if (fullRedraw) requestFullUIRedraw();
  else requestUIRedraw();
  clearInputLatch();
}

// “Synced enough” for our purposes: valid epoch beyond a floor.
static inline bool timeSyncedNow()
{
  time_t now = time(nullptr);
  return (now > 1700000000); // ~late 2023
}

static void bootWizardSkipToManualTime()
{
  s_bootWifiWizardActive    = false;
  g_wifiSetupFromBootWizard = false;

  // This helper already enters SET_TIME + handles return bookkeeping.
  beginForcedSetTimeBootGate(s_afterState, s_afterTab);

  requestFullUIRedraw();
  clearInputLatch();
}

void bootWizardBegin(UIState afterState, Tab afterTab)
{
  s_bootWifiWizardActive = true;
  s_afterState = afterState;
  s_afterTab   = afterTab;

  // Boot wizard screens always render in the pet tab UI.
  gotoState(UIState::BOOT_WIFI_PROMPT, Tab::TAB_PET, true);
}

static void handleBootWifiPromptInput(InputState& in)
{
  if (in.escOnce || in.menuOnce)
  {
    bootWizardSkipToManualTime();
    return;
  }

  if (in.selectOnce || in.encoderPressOnce)
  {
    g_wifiSetupFromBootWizard = true;
    g_wifi.setupStage = 0;
    g_wifi.buf[0]     = '\0';
    g_wifi.ssid[0]    = '\0';
    g_wifi.pass[0]    = '\0';

    // Router will enable text capture automatically for WIFI_SETUP.
    gotoState(UIState::WIFI_SETUP, g_app.currentTab, false);

    drainKb(in);
    clearInputLatch();
    playBeep();
    return;
  }

  drainKb(in);
  clearInputLatch();
}

static void handleBootWifiWaitInput(InputState& in)
{
  if (in.escOnce || in.menuOnce)
  {
    bootWizardSkipToManualTime();
    return;
  }

  if (wifiIsConnectedNow())
  {
    gotoState(UIState::BOOT_TZ_PICK, g_app.currentTab, true);
    return;
  }

  drainKb(in);
  clearInputLatch();
}

static void handleBootTzPickInput(InputState& in)
{
  // ESC on TZ pick: allow user to skip TZ selection and proceed to NTP wait
  if (in.escOnce || in.menuOnce)
  {
    gotoState(UIState::BOOT_NTP_WAIT, g_app.currentTab, false);
    return;
  }

  int mv = in.encoderDelta;
  if (in.upOnce)   mv = -1;
  if (in.downOnce) mv =  1;

  if (mv != 0)
  {
    settingsCycleTimeZone(mv < 0 ? -1 : +1);
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  if (in.selectOnce || in.encoderPressOnce)
  {
    saveSettingsToSD();
    saveTzIndexToNVS((uint8_t)tzIndex);
    applyTimezoneIndex((uint8_t)tzIndex);

    gotoState(UIState::BOOT_NTP_WAIT, g_app.currentTab, false);
    return;
  }

  drainKb(in);
  clearInputLatch();
}

static void handleBootNtpWaitInput(InputState& in)
{
  if (in.escOnce || in.menuOnce)
  {
    bootWizardSkipToManualTime();
    return;
  }

  if (wifiIsConnectedNow() && timeSyncedNow() && timeIsValid())
  {
    saveTimeAnchor();
    updateTime();

    s_bootWifiWizardActive    = false;
    g_wifiSetupFromBootWizard = false;

    // If returning to egg select, require clean release before hatch
    if (s_afterState == UIState::CHOOSE_PET)
    {
      g_choosePetBlockHatchUntilRelease = true;
    }

    gotoState(s_afterState, s_afterTab, true);
    return;
  }

  drainKb(in);
  clearInputLatch();
}

void uiBootWifiPromptHandle(InputState& in)
{
  handleBootWifiPromptInput(in);
}

void uiBootWifiWaitHandle(InputState& in)
{
  handleBootWifiWaitInput(in);
}

void uiBootTzPickHandle(InputState& in)
{
  handleBootTzPickInput(in);
}

void uiBootNtpWaitHandle(InputState& in)
{
  handleBootNtpWaitInput(in);
}