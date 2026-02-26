#include "ui_state_mini_game.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "mini_games.h"
#include "mg_pause_menu.h"

void uiMiniGameHandle(InputState& in)
{
  // ESC opens mini-game pause menu
  if (in.mgQuitOnce) {
    const uint32_t now = millis();
    mgPauseSetPaused(true);
    mgPauseSetChoice(0);
    mgPauseUpdateClocks(now);

    g_app.uiState = UIState::MG_PAUSE;
    clearInputLatch();
    requestUIRedraw();
    return;
  }

  updateMiniGame(in);
  requestUIRedraw();
}