#include "ui_state_sleep_menu.h"

#include <Arduino.h>

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "pet.h"
#include "save_manager.h"
#include "sleep_state.h"
#include "ui_input_utils.h"
#include "ui_menu_state.h"
#include "ui_suppress.h"

void uiSleepMenuHandle(InputState& in)
{
  const int totalItems = 4;

  const int move = uiNavMove(in);
  if (move != 0) {
    uiWrapIndex(sleepMenuIndex, move, totalItems);
    requestUIRedraw();
    return;
  }

  if (uiIsBack(in)) {
    g_app.uiState    = UIState::PET_SCREEN;
    g_app.currentTab = Tab::TAB_PET;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  if (!uiIsSelect(in)) return;

  auto enterSleep = [&]() {
    pet.isSleeping   = true;
    g_app.isSleeping = true;
    g_app.uiState    = UIState::PET_SLEEPING;
    g_app.currentTab = Tab::TAB_PET;
    requestUIRedraw();

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
      break;

    case 1: // Until Rested
      g_app.sleepUntilRested   = true;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 0;
      break;

    case 2: // 4 hours
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 4UL * 60UL * 60UL * 1000UL;
      break;

    case 3: // 8 hours
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 8UL * 60UL * 60UL * 1000UL;
      break;
  }

  enterSleep();
  clearInputLatch();
}