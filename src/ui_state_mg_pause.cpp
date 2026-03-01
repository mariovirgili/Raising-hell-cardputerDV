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

  // Drive the pause menu interaction
  const uint8_t r = mgPauseHandle(in);

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

  // Still paused: let the pause menu render path handle it.
}