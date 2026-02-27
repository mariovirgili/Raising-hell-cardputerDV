#include "ui_state_mg_pause.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "mg_pause_menu.h"
#include "mini_games.h"
#include <Arduino.h>
#include "ui_suppress.h"

void uiMgPauseHandle(InputState &in)
{
  const uint32_t now = millis();

  // ESC resumes back to MINI_GAME
  if (in.mgQuitOnce)
  {
    mgPauseSetPaused(false);
    mgPauseUpdateClocks(now);

    // Prevent the ESC edge from triggering something immediately after resume
    uiGuardTransition(in);

    g_app.uiState = UIState::MINI_GAME;
    requestUIRedraw();
    return;
  }

  // Navigate (0=Continue, 1=Exit)
  if (in.mgUpOnce || in.mgLeftOnce)
  {
    mgPauseSetChoice(0);
    requestUIRedraw();
    return;
  }
  if (in.mgDownOnce || in.mgRightOnce)
  {
    mgPauseSetChoice(1);
    requestUIRedraw();
    return;
  }

  // Confirm (ENTER)
  if (in.mgSelectOnce)
  {
    const uint8_t choice = mgPauseChoice();

    if (choice == 0) // Continue
    {
      mgPauseSetPaused(false);
      mgPauseUpdateClocks(now);

      // Kill the ENTER edge so it doesn't double-trigger in MINI_GAME
      uiGuardTransition(in);

      g_app.uiState = UIState::MINI_GAME;
      requestUIRedraw();
      return;
    }

    if (choice == 1) // Exit
    {
      mgPauseSetPaused(false);
      mgPauseUpdateClocks(now);
      mgPauseSetChoice(0);

      // CRITICAL: kill the ENTER edge BEFORE we return to the PET/PLAY UI,
      // otherwise the Play handler sees it and starts the minigame again.
      uiGuardTransition(in);

      miniGameExitToReturnUi(true);
      return;
    }
  }
}