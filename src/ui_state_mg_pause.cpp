#include "ui_state_mg_pause.h"

#include "app_state.h"
#include "input.h"
#include "mini_games.h"
#include "ui_actions.h"
#include "ui_runtime.h"
#include "ui_suppress.h"

// IMPORTANT: MGPAUSE_* return codes + mgPauseHandle()/mgPauseChoice() live here
#include "mg_pause_menu.h"

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

  // If we somehow got here while not paused:
  // - If a mini-game is still running, go back to MINI_GAME.
  // - If not, DO NOT bounce through MINI_GAME (that's what causes "NO MINI GAME" flash).
  if (!mgPauseIsPaused())
  {
    if (g_app.inMiniGame && currentMiniGame != MiniGame::NONE)
    {
      uiActionEnterStateClean(UIState::MINI_GAME, g_app.currentTab, false, in, 150);
      requestFullUIRedraw();
    }
    return;
  }

  const uint8_t beforeChoice = mgPauseChoice();

  // Drive the pause menu interaction.
  // NOTE: In your codebase, selecting "Exit" triggers miniGameExitToReturnUi(true)
  // from inside the pause-menu action handler. So we must not force a MINI_GAME hop here.
  const uint8_t r = mgPauseHandle(in);

  // Selection highlight should redraw instantly.
  if (mgPauseChoice() != beforeChoice)
  {
    requestUIRedraw();
  }

  // If the user chose Exit, the action already performed the exit+state change.
  // Do NOT force a transition to MINI_GAME here.
  if (r == MGPAUSE_EXIT)
  {
    requestFullUIRedraw();
    return;
  }

  // If we're still paused, we stay in MG_PAUSE and let the draw path render overlay.
  // If we unpaused via "Continue", mgPauseIsPaused() will become false and the next tick
  // will naturally return to MINI_GAME via the early check above (only if game still exists).
}