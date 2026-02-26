#include "ui_state_sleep_menu.h"

#include "app_state.h"
#include "input.h"
#include "ui_input_utils.h"
#include "ui_menu_state.h"
#include "ui_actions.h"
#include "ui_sleep_menu.h"
#include "ui_runtime.h"

void uiSleepMenuHandle(InputState& in)
{
  const int totalItems = uiSleepMenuCount();

  const int move = uiNavMove(in);
  if (move != 0) {
    uiWrapIndex(sleepMenuIndex, move, totalItems);
    requestUIRedraw();
    return;
  }

  if (uiIsBack(in)) {
    uiActionEnterState(UIState::PET_SCREEN, Tab::TAB_PET, true);
    clearInputLatch();
    return;
  }

  if (!uiIsSelect(in)) return;

  // All behavior lives in ui_sleep_menu_actions via ui_sleep_menu.cpp mapping.
  (void)uiSleepMenuActivate(sleepMenuIndex, in);

  inputForceClear();
  clearInputLatch();
}