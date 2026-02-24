#include "ui_state_shop.h"

#include "app_state.h"
#include "input.h"
#include "shop_actions.h"
#include "shop_items.h"
#include "sound.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"

void uiShopHandle(InputState& in)
{
  if (in.menuOnce) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  int move = in.encoderDelta;
  if (in.upOnce) move = -1;
  if (in.downOnce) move = 1;

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

  if (in.selectOnce || in.encoderPressOnce) {
    shopBuyItem();
    requestUIRedraw();
    clearInputLatch();
  }
}