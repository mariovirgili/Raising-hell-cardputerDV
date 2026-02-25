// flow_controls_help.cpp
#include "flow_controls_help.h"

#include <Arduino.h>

#include "app_state.h"
#include "controls_help_state.h"
#include "display.h"
#include "graphics.h"
#include "input.h"
#include "settings_flow_state.h"
#include "ui_actions.h"
#include "ui_runtime.h"
#include "ui_suppress.h"

// Controls Help return target (so Settings->Controls can return back to Settings)
static bool s_returnToSettings = false;
static SettingsPage s_returnPage = SettingsPage::TOP;
static int s_returnTopIndex = 0;

void openControlsHelpFromSettings()
{
  s_returnToSettings = true;
  s_returnPage = SettingsPage::TOP;
  s_returnTopIndex = g_app.settingsIndex;

  g_app.uiState = UIState::CONTROLS_HELP;
  requestFullUIRedraw();

  // Keep it non-sticky
  clearInputLatch();
  inputForceClear();
}

void openControlsHelpFromAnywhere()
{
  if (g_app.uiState == UIState::CONTROLS_HELP)
    return;

  // If we're in Settings, preserve the current Settings cursor via the existing path.
  if (g_app.uiState == UIState::SETTINGS)
  {
    openControlsHelpFromSettings();
    return;
  }

  // Otherwise, capture current screen/tab and open the overlay.
  // This is intentionally non-interrupting: it does not touch pet sleep state.
  s_returnToSettings = false;
  controlsHelpBegin(g_app.uiState, g_app.currentTab);
  g_app.uiState = UIState::CONTROLS_HELP;
  requestFullUIRedraw();

  clearInputLatch();
  inputForceClear();
}

static void handleControlsHelpInput(InputState &in)
{
  bool kbDismiss = false;

  // Treat keyboard Enter / Backspace as dismiss too
  while (in.kbHasEvent())
  {
    const KeyEvent ev = in.kbPop();
    const uint8_t c = ev.code;

    // Enter can arrive as '\n' (10) or '\r' (13) depending on source
    // Backspace is '\b' (8)
    if (c == '\n' || c == '\r' || c == '\b')
    {
      kbDismiss = true;
    }
  }
  // Any of these dismisses
  if (kbDismiss || in.selectOnce || in.encoderPressOnce || in.escOnce || in.menuOnce)
  {
    controlsHelpDismiss();

    uiSuppressMenuForMs(250);
    uiActionDrainKb(in);
    in.clearEdges();
    clearInputLatch();

    if (s_returnToSettings)
    {
      s_returnToSettings = false;

      g_app.uiState = UIState::SETTINGS;
      g_settingsFlow.settingsPage = s_returnPage;
      g_app.settingsIndex = s_returnTopIndex;

      requestFullUIRedraw();

      inputForceClear();
      uiActionSwallowEdges(in);
      clearInputLatch();
    }

    return;
  }

  // otherwise swallow everything (no accidental actions)
  clearInputLatch();
}

void uiControlsHelpHandle(InputState &in) { handleControlsHelpInput(in); }