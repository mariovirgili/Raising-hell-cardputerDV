#include "ui_state_mini_game.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "mini_games.h"

void uiMiniGameHandle(InputState& in)
{
  // Preserve legacy behavior:
  // - MENU exits mini-game back to pet screen
  // - update runs every tick (even with no edges)
  if (in.menuOnce) {
    g_app.inMiniGame    = false;
    currentMiniGame     = MiniGame::NONE;
    g_app.uiState       = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  updateMiniGame(in);
  requestUIRedraw();
}