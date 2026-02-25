#include "ui_state_shop.h"

#include "app_state.h"
#include "input.h"
#include "shop_actions.h"
#include "shop_items.h"
#include "sound.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"
#include "ui_input_common.h"

void uiShopHandle(InputState& in)
{
  if (in.menuOnce) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  const int move = uiNavMove(in);
  if (move != 0) {
    const int totalItems = SHOP_ITEM_COUNT; // no Exit pill
    if (totalItems > 0) {
      // True round-robin, supports move magnitudes > 1
      shopIndex = (shopIndex + move) % totalItems;
      if (shopIndex < 0) shopIndex += totalItems;

      requestUIRedraw();
      playBeep();
    }
    return;
  }

  if (uiSelectPressed(in)) {
    shopBuyItem();
    requestUIRedraw();
    clearInputLatch();
  }
}