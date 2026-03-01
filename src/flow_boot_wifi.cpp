#include "flow_boot_wifi.h"

#include <string.h>

#include "app_state.h"
#include "input.h"
#include "sound.h"
#include "ui_actions.h"
#include "ui_input_common.h"
#include "ui_runtime.h"      // requestUIRedraw()

#include "flow_time_editor.h"
#include "wifi_setup_state.h"
#include "wifi_time.h"
#include "timezone.h"

// These are defined in flow_boot_wizard.cpp
extern UIState g_bootWizardAfterOkState;
extern Tab     g_bootWizardAfterOkTab;

// -----------------------------------------------------------------------------
// Local wizard state
// -----------------------------------------------------------------------------
static bool s_chooseWifi = true; // true = WiFi/NTP path, false = manual time

static void bootWizardSkipToManualTime()
{
  // Go straight to mandatory time set gate; cannot cancel.
  beginForcedSetTimeBootGate(g_bootWizardAfterOkState, g_bootWizardAfterOkTab);
  requestUIRedraw();
}

static void bootWizardEnterWifiSetup()
{
  // Mark that we're coming from boot wizard (used by WIFI_SETUP state logic)
  g_wifiSetupFromBootWizard = true;

  // Reset buffers/stage using the legacy ref aliases
  wifiSetupStage = 0;
  wifiSetupSsid[0] = 0;
  wifiSetupPass[0] = 0;
  wifiSetupBuf[0]  = 0;

  // Enter the existing WiFi setup UI
  uiActionEnterState(UIState::WIFI_SETUP, g_bootWizardAfterOkTab, true);
  requestUIRedraw();
}

// -----------------------------------------------------------------------------
// BOOT_WIFI_PROMPT
// -----------------------------------------------------------------------------
void uiBootWifiPromptHandle(InputState& in)
{
  // ESC skips WiFi and goes straight to manual time setup.
  if (in.escOnce)
  {
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    bootWizardSkipToManualTime();
    return;
  }

  // MENU inactive here (don’t open Settings during boot wizard)
  if (in.menuOnce)
  {
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    return;
  }

  // Up/Down toggles choice
  if (in.upOnce || in.downOnce)
  {
    s_chooseWifi = !s_chooseWifi;
    requestUIRedraw();
    uiActionSwallowAll(in);
    return;
  }

  // Enter confirms
  if (in.selectOnce)
  {
    if (s_chooseWifi)
      bootWizardEnterWifiSetup();
    else
      bootWizardSkipToManualTime();

    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    return;
  }

  uiActionSwallowAll(in);
}

// -----------------------------------------------------------------------------
// BOOT_WIFI_WAIT
// Show “connecting…” after WiFi setup returns; once connected, advance.
// ESC always skips to manual.
// -----------------------------------------------------------------------------
void uiBootWifiWaitHandle(InputState& in)
{
  if (in.escOnce || in.menuOnce)
  {
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    bootWizardSkipToManualTime();
    return;
  }

  // If WiFi is connected, proceed to TZ pick
  if (wifiIsConnected())
  {
    uiActionEnterState(UIState::BOOT_TZ_PICK, g_bootWizardAfterOkTab, true);
    requestUIRedraw();
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    return;
  }

  uiActionSwallowAll(in);
}

// -----------------------------------------------------------------------------
// BOOT_TZ_PICK
// Pick timezone using tzIndex + tzCount/tzName, commit, then go wait for NTP.
// -----------------------------------------------------------------------------
static void tzPrev()
{
  const uint8_t n = tzCount();
  if (n == 0) return;
  if (tzIndex <= 0) tzIndex = (int)n - 1;
  else tzIndex--;
}

static void tzNext()
{
  const uint8_t n = tzCount();
  if (n == 0) return;
  tzIndex++;
  if (tzIndex >= (int)n) tzIndex = 0;
}

static void tzCommit()
{
  applyTimezoneIndex((uint8_t)tzIndex);
  saveTzIndexToNVS((uint8_t)tzIndex);
}

void uiBootTzPickHandle(InputState& in)
{
  if (in.escOnce || in.menuOnce)
  {
    // allow skip to manual time
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    bootWizardSkipToManualTime();
    return;
  }

  if (in.upOnce)
  {
    tzPrev();
    requestUIRedraw();
    uiActionSwallowAll(in);
    return;
  }

  if (in.downOnce)
  {
    tzNext();
    requestUIRedraw();
    uiActionSwallowAll(in);
    return;
  }

  if (in.selectOnce)
  {
    tzCommit();
    uiActionEnterState(UIState::BOOT_NTP_WAIT, g_bootWizardAfterOkTab, true);
    requestUIRedraw();
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    return;
  }

  uiActionSwallowAll(in);
}

// -----------------------------------------------------------------------------
// BOOT_NTP_WAIT
// Wait for time sync; ESC skips to manual. Once synced, continue.
// -----------------------------------------------------------------------------
void uiBootNtpWaitHandle(InputState& in)
{
  if (in.escOnce || in.menuOnce)
  {
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    bootWizardSkipToManualTime();
    return;
  }

  if (timeIsSynced())
  {
    uiActionEnterState(g_bootWizardAfterOkState, g_bootWizardAfterOkTab, true);
    requestUIRedraw();
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    return;
  }

  uiActionSwallowAll(in);
}