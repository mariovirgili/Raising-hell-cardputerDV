#include "ui_state_mg_pause.h"

#include <Arduino.h>
#include "app_state.h"
#include "input.h"
#include "mini_games.h"
#include "mg_pause_menu.h"
#include "graphics.h"

void uiMgPauseHandle(InputState& in)
{
  // ESC resumes back to MINI_GAME
  if (in.mgQuitOnce) {
    const uint32_t now = millis();
    mgPauseSetPaused(false);
    mgPauseUpdateClocks(now);

    g_app.uiState = UIState::MINI_GAME;
    clearInputLatch();
    requestUIRedraw();
    return;
  }

  // Navigate (0=Continue, 1=Exit)
  if (in.mgUpOnce || in.mgLeftOnce) {
    mgPauseSetChoice(0);
    requestUIRedraw();
    return;
  }
  if (in.mgDownOnce || in.mgRightOnce) {
    mgPauseSetChoice(1);
    requestUIRedraw();
    return;
  }

  // Confirm (ENTER)
  if (in.mgSelectOnce) {
    const uint8_t choice = mgPauseChoice();
    if (choice == 0) {
      const uint32_t now = millis();
      mgPauseSetPaused(false);
      mgPauseUpdateClocks(now);

      g_app.uiState = UIState::MINI_GAME;
      clearInputLatch();
      requestUIRedraw();
      return;
    } else {
      // Exit mini-game using your existing path
      miniGameExitToReturnUi(true);
      return;
    }
  }
}