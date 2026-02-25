#include "menu_actions.h"

#include "app_state.h"
#include "input.h"
#include "led_status.h"
#include "mini_games.h"
#include "pet.h"
#include "save_manager.h"
#include "sound.h"
#include "ui_input_router.h"
#include "ui_invalidate.h"
#include "ui_runtime.h"

#include "death_state.h"
#include "graphics.h"  // invalidateBackgroundCache()

// ==================================================================
// MAIN DISPATCHER (thin wrapper)
// ==================================================================
bool handleMenuInput(InputState &in)
{
  const UIState oldState = g_app.uiState;
  uiHandleInput(in);
  return (oldState != g_app.uiState);
}

// ==================================================================
// RESURRECTION MINI-GAME RESULT (called by mini_games.cpp)
// ==================================================================
void onResurrectionMiniGameResult(bool success)
{
  if (success)
  {
    petResurrectFull();

    currentMiniGame  = MiniGame::NONE;
    g_app.inMiniGame = false;
    g_app.gameOver   = false;

    g_app.currentTab = Tab::TAB_PET;
    g_app.uiState    = UIState::PET_SCREEN;

    requestUIRedraw();
    invalidateBackgroundCache();

    inputForceClear();
    clearInputLatch();
    requestUIRedraw();
    return;
  }

  // Failed: return to death screen
  resetDeathMenu();
  g_app.uiState = UIState::DEATH;

  requestUIRedraw();
  invalidateBackgroundCache();

  inputForceClear();
  clearInputLatch();
  requestUIRedraw();
}

// ==================================================================
// SETTINGS: LED alerts toggle (called from settings actions)
// ==================================================================
void settingsToggleLedAlerts()
{
  ledAlertsEnabled = !ledAlertsEnabled;

  if (!ledAlertsEnabled)
  {
    ledUpdatePetStatus(LED_PET_OFF);
  }

  saveSettingsToSD();
  saveManagerMarkDirty();

  requestUIRedraw();
  playBeep();
  clearInputLatch();
}