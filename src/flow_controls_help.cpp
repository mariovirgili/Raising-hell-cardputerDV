// flow_controls_help.cpp
#include "flow_controls_help.h"

#include <Arduino.h>

#include "app_state.h"
#include "controls_help_state.h"
#include "input.h"
#include "menu_actions_internal.h"
#include "settings_flow_state.h"
#include "ui_runtime.h"
#include "display.h"
#include "graphics.h"

// Controls Help return target (so Settings->Controls can return back to Settings)
static bool         s_returnToSettings = false;
static SettingsPage s_returnPage       = SettingsPage::TOP;
static int          s_returnTopIndex   = 0;

// Small helper to drain queued key events safely
static inline void drainKb(InputState& in) {
  while (in.kbHasEvent()) (void)in.kbPop();
}

void openControlsHelpFromSettings() {
  s_returnToSettings = true;
  s_returnPage       = SettingsPage::TOP;
  s_returnTopIndex   = g_app.settingsIndex;

  g_app.uiState = UIState::CONTROLS_HELP;
  requestFullUIRedraw();

  // Keep it non-sticky
  clearInputLatch();
  inputForceClear();
}

void openControlsHelpFromAnywhere() {
  if (g_app.uiState == UIState::CONTROLS_HELP) return;

  // If we're in Settings, preserve the current Settings cursor via the existing path.
  if (g_app.uiState == UIState::SETTINGS) {
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

void handleControlsHelpInput(InputState& in) {
  // Any of these dismisses
  if (in.selectOnce || in.encoderPressOnce || in.escOnce || in.menuOnce) {
    controlsHelpDismiss();

    // Prevent the same dismiss key from being re-consumed next tick
    // (e.g., ESC/MENU immediately opening Settings or advancing another screen).
    menuActionsSuppressMenuForMs(250);

    // Consume current-tick edges too.
    in.clearEdges();

    // swallow any residue
    drainKb(in);
    clearInputLatch();

    // If we opened this from Settings->Controls, return there deterministically
    if (s_returnToSettings) {
      s_returnToSettings = false;

      g_app.uiState                = UIState::SETTINGS;
      g_settingsFlow.settingsPage  = s_returnPage;
      g_app.settingsIndex          = s_returnTopIndex;

      requestFullUIRedraw();

      inputForceClear();
      clearInputLatch();
    }

    return;
  }

  // otherwise swallow everything (no accidental actions)
  drainKb(in);
  clearInputLatch();
}
