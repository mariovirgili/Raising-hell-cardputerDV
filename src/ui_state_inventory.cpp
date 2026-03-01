#include "ui_state_inventory.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "sound.h"
#include "ui_actions.h"
#include "ui_input_utils.h"
#include "ui_runtime.h"

void uiInventoryHandle(InputState& in)
{
  // Back goes to pet tab/state.
  if (uiIsBack(in))
  {
    uiActionEnterStateClean(UIState::PET_SCREEN, Tab::TAB_PET, false, in, 150);
    requestFullUIRedraw();
    clearInputLatch();
    return;
  }

  // Navigate visible inventory entries.
  const int move = uiNavMove(in);
  if (move != 0)
  {
    const int count = g_app.inventory.countItems();
    if (count > 0)
    {
      uiWrapIndex(g_app.inventory.selectedIndex, move, count);
      requestUIRedraw();
      playBeep();
    }
    clearInputLatch();
    return;
  }

  // Use selected item.
  if (uiIsSelect(in))
  {
    g_app.inventory.useSelectedItem();
    requestUIRedraw();
    clearInputLatch();
  }
}