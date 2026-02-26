#include "ui_state_shop.h"

#include "app_state.h"
#include "input.h"
#include "sound.h"
#include "ui_actions.h"
#include "ui_input_utils.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"
#include "ui_shop_menu.h"

void uiShopHandle(InputState& in)
{
  if (uiIsBack(in)) {
    uiActionEnterState(UIState::PET_SCREEN, Tab::TAB_PET, true);
    clearInputLatch();
    return;
  }

  const int move = uiNavMove(in);
  if (move != 0) {
    const int totalItems = uiShopMenuCount();
    if (totalItems > 0) {
      uiWrapIndex(shopIndex, move, totalItems);
      requestUIRedraw();
      playBeep();
    }
    return;
  }

  if (!uiIsSelect(in)) return;

  (void)uiShopMenuActivate(shopIndex, in);
  requestUIRedraw();
  clearInputLatch();
}