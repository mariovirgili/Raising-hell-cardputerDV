#include "flow_boot_wifi.h"

#include "app_state.h"
#include "input.h"
#include "ui_actions.h"
#include "ui_runtime.h"
#include "ui_input_common.h"   // uiDrainKb()

#include "wifi_time.h"
#include "flow_time_editor.h"

#include "ui_boot_wizard_menu.h"

// These are defined in flow_boot_wizard.cpp
extern UIState g_bootWizardAfterOkState;
extern Tab     g_bootWizardAfterOkTab;

// -----------------------------------------------------------------------------
// BOOT_WIFI_PROMPT
// -----------------------------------------------------------------------------
void uiBootWifiPromptHandle(InputState& in)
{
  (void)UiBootWizardMenu::HandleWifiPrompt(in);
}

// -----------------------------------------------------------------------------
// BOOT_WIFI_WAIT
// After WiFi setup returns, wait for connection then advance.
// ESC always skips to manual time.
// -----------------------------------------------------------------------------
void uiBootWifiWaitHandle(InputState& in)
{
  if (in.escOnce || in.menuOnce)
  {
    uiActionSwallowAll(in);
    uiDrainKb(in);
    clearInputLatch();
    beginForcedSetTimeBootGate(g_bootWizardAfterOkState, g_bootWizardAfterOkTab);
    requestUIRedraw();
    return;
  }

  if (wifiIsConnected())
  {
    // Advance to TZ pick after connection is established
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
// -----------------------------------------------------------------------------
void uiBootTzPickHandle(InputState& in)
{
  (void)UiBootWizardMenu::HandleTimezonePick(in);
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
    beginForcedSetTimeBootGate(g_bootWizardAfterOkState, g_bootWizardAfterOkTab);
    requestUIRedraw();
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