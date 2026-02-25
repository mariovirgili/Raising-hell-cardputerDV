#include "ui_state_inventory.h"

#include "app_state.h"
#include "input.h"
#include "save_manager.h"
#include "sound.h"
#include "ui_input_utils.h"
#include "ui_runtime.h"

void uiInventoryHandle(InputState& in)
{
  if (uiIsBack(in)) {
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
    uiWrapIndex(g_app.inventory.selectedIndex, move, count);
    requestUIRedraw();
    playBeep();
    return;
  }

  if (uiIsSelect(in)) {
    g_app.inventory.useSelectedItem();
    saveManagerMarkDirty();
    requestUIRedraw();
    clearInputLatch();
  }
}