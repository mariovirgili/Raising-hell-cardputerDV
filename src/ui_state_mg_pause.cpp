#include "ui_state_mg_pause.h"

#include "app_state.h"
#include "input.h"
#include "mg_pause_core.h"
#include "mg_pause_menu.h"
#include "mini_games.h"
#include "ui_actions.h"
#include "ui_runtime.h"
#include "ui_suppress.h"

void uiMgPauseEnter()
{
  // Ensure we are in the paused state and the cursor starts at "Continue".
  mgPauseSetPaused(true);
  mgPauseSetChoice(0);

  requestFullUIRedraw();
}

void uiMgPauseHandle(InputState& in)
{
  // Keep menu suppressed while we're in the pause UI (ESC should resume game, not open Settings).
  uiSuppressMenuForMs(150);

  // If we somehow got here while not paused, immediately return to mini game.
  if (!mgPauseIsPaused())
  {
    uiActionEnterStateClean(UIState::MINI_GAME, g_app.currentTab, false, in, 150);
    requestFullUIRedraw();
    return;
  }

  const uint8_t before = mgPauseChoice();

  // Drive the pause menu interaction
  const uint8_t r = mgPauseHandle(in);

  // If selection changed, redraw immediately so navigation feels snappy.
  if (mgPauseChoice() != before)
  {
    requestUIRedraw();
  }

  if (r == MGPAUSE_EXIT)
  {
    miniGameExitToReturnUi(true);
    requestFullUIRedraw();
    return;
  }

  // Continue: mgPauseHandle() should have cleared pause via action
  if (!mgPauseIsPaused())
  {
    uiActionEnterStateClean(UIState::MINI_GAME, g_app.currentTab, false, in, 150);
    requestFullUIRedraw();
    return;
  }

  // Still paused: draw path will render MG_PAUSE state.
}