#include "ui_state_inventory.h"

#include "app_state.h"
#include "input.h"
#include "save_manager.h"
#include "sound.h"
#include "ui_runtime.h"
#include "ui_input_common.h"

void uiInventoryHandle(InputState& in)
{
  // Preserve existing behavior: MENU exits inventory back to pet screen.
  if (in.menuOnce) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  const int count = g_app.inventory.countItems();
  if (count == 0) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  const int move = uiNavMove(in);
  if (move != 0) {
    g_app.inventory.selectedIndex += move;
    uiWrapIndex(g_app.inventory.selectedIndex, count);

    requestUIRedraw();
    playBeep();
    return;
  }

  if (uiSelectPressed(in)) {
    g_app.inventory.useSelectedItem();
    saveManagerMarkDirty();
    requestUIRedraw();
    clearInputLatch();
  }
}