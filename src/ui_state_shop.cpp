#include "ui_state_shop.h"

#include "app_state.h"
#include "input.h"
#include "shop_actions.h"
#include "shop_items.h"
#include "sound.h"
#include "ui_input_utils.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"

void uiShopHandle(InputState& in)
{
  if (uiIsBack(in)) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  const int move = uiNavMove(in);
  if (move != 0) {
    const int totalItems = SHOP_ITEM_COUNT;
    if (totalItems > 0) {
      uiWrapIndex(shopIndex, move, totalItems);
      requestUIRedraw();
      playBeep();
    }
    return;
  }

  if (uiIsSelect(in)) {
    shopBuyItem();
    requestUIRedraw();
    clearInputLatch();
  }
}