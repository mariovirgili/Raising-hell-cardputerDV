#include "ui_state_inventory.h"

#include "app_state.h"
#include "input.h"
#include "save_manager.h"
#include "sound.h"
#include "ui_runtime.h"

void uiInventoryHandle(InputState& in)
{
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

  int move = in.encoderDelta;
  if (in.upOnce) move = -1;
  if (in.downOnce) move = 1;

  if (move != 0) {
    g_app.inventory.selectedIndex += move;
    if (g_app.inventory.selectedIndex < 0) g_app.inventory.selectedIndex = count - 1;
    if (g_app.inventory.selectedIndex >= count) g_app.inventory.selectedIndex = 0;

    requestUIRedraw();
    playBeep();
    return;
  }

  if (in.selectOnce || in.encoderPressOnce) {
    g_app.inventory.useSelectedItem();
    saveManagerMarkDirty();
    requestUIRedraw();
    clearInputLatch();
  }
}