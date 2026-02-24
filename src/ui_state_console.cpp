#include "ui_state_console.h"

#include "app_state.h"
#include "console.h"
#include "input.h"
#include "settings_flow_state.h"
#include "ui_input_utils.h"
#include "ui_runtime.h"

// From menu_actions.cpp: prevents menu/esc from bouncing immediately after returning.
void menuActionsSuppressMenuForMs(uint32_t ms);

// Console return target
static UIState      g_consoleReturnState      = UIState::PET_SCREEN;
static Tab          g_consoleReturnTab        = Tab::TAB_PET;
static bool         g_consoleReturnToSettings = false;
static SettingsPage g_consoleReturnPage       = SettingsPage::TOP;

void openConsoleWithReturn(UIState returnState, Tab returnTab, bool retToSettings, SettingsPage retSettingsPage) {
  g_consoleReturnState      = returnState;
  g_consoleReturnTab        = returnTab;
  g_consoleReturnToSettings = retToSettings;
  g_consoleReturnPage       = retSettingsPage;

  consoleOpen();
  g_app.uiState = UIState::CONSOLE;
  requestUIRedraw();
}

static inline void swallowTypingAndEdges(InputState& in) {
  // If we just returned from text-entry states, the key buffer can still have junk.
  uiDrainKb(in);

  // Clear one-shot edges so we don't immediately close/submit.
  in.escOnce = false;
  in.menuOnce = false;
  in.selectOnce = false;
  in.encoderPressOnce = false;

  clearInputLatch();
}

void uiConsoleHandle(InputState& input) {
  if (input.menuOnce || input.escOnce) {
    consoleClose();

    if (g_consoleReturnToSettings) {
      g_settingsFlow.settingsReturnValid = true;
      g_settingsFlow.settingsReturnState = g_consoleReturnState;
      g_settingsFlow.settingsReturnTab   = g_consoleReturnTab;
      g_settingsFlow.settingsPage        = g_consoleReturnPage;

      g_app.uiState = UIState::SETTINGS;
      requestUIRedraw();
      swallowTypingAndEdges(input);
      menuActionsSuppressMenuForMs(250);
      return;
    }

    g_app.uiState    = g_consoleReturnState;
    g_app.currentTab = g_consoleReturnTab;
    requestUIRedraw();
    swallowTypingAndEdges(input);
    menuActionsSuppressMenuForMs(250);
    return;
  }

  // Let the console module handle keystrokes, cursor, etc.
  consoleUpdate(input);
  requestUIRedraw();
}