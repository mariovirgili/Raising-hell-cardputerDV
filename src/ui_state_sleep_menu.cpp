#include "ui_state_sleep_menu.h"

#include <Arduino.h>

#include "app_state.h"
#include "graphics.h" 
#include "input.h"
#include "pet.h"
#include "save_manager.h"
#include "sleep_state.h"
#include "ui_menu_state.h"
#include "ui_input_common.h"

// defined in menu_actions.cpp today; we keep using the same suppression timer
extern void menuActionsSuppressMenuForMs(uint32_t durationMs);

// this exists in menu_actions.cpp today as a static; we need our own in-module suppress
static uint32_t g_suppressSleepWakeUntilMs_local = 0;

void uiSleepMenuHandle(InputState& in)
{
  const int totalItems = 4;

  // Navigation (unified)
  const int move = uiNavMove(in);
  if (move != 0) {
    sleepMenuIndex += move;
    uiWrapIndex(sleepMenuIndex, totalItems);
    requestUIRedraw();
    return;
  }

  // Back (unified) — same behavior as before
  if (uiBackPressed(in)) {
    g_app.uiState    = UIState::PET_SCREEN;
    g_app.currentTab = Tab::TAB_PET;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  // Select (unified)
  if (!uiSelectPressed(in)) return;

  auto enterSleep = [&]() {
    pet.isSleeping         = true;
    g_app.isSleeping       = true;
    g_app.uiState          = UIState::PET_SLEEPING;
    g_app.currentTab       = Tab::TAB_PET;
    requestUIRedraw();

    // prevent the ENTER that starts sleep from waking instantly
    g_suppressSleepWakeUntilMs_local = millis() + 400;

    inputForceClear();
    clearInputLatch();

    g_app.sleepTargetEnergy = 0;
    invalidateBackgroundCache();
    saveManagerMarkDirty();

    sleepBgKickNow();
  };

  switch (sleepMenuIndex) {
    case 0: // Until Awakened
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = true;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 0;
      enterSleep();
      break;

    case 1: // Until Rested
      g_app.sleepUntilRested   = true;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 0;
      enterSleep();
      break;

    case 2: // 4 hours
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 4UL * 60UL * 60UL * 1000UL;
      enterSleep();
      break;

    case 3: // 8 hours
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 8UL * 60UL * 60UL * 1000UL;
      enterSleep();
      break;
  }

  clearInputLatch();
}