#include "ui_state_death.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "mini_games.h"
#include "sound.h"
#include "ui_input_common.h"
#include "ui_input_utils.h"
#include "ui_runtime.h"

extern int deathMenuIndex;

void uiDeathHandle(InputState& in)
{
  const int move = uiNavMove(in);
  if (move != 0) {
    uiWrapIndex(deathMenuIndex, move, 2);
    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (!uiIsSelect(in)) return;

  clearInputLatch();
  inputForceClear();

  if (deathMenuIndex == 0) {
    g_app.inMiniGame = true;
    g_app.gameOver   = false;
    g_app.uiState    = UIState::MINI_GAME;
    requestUIRedraw();

    invalidateBackgroundCache();

    startResurrectionRun();
    currentMiniGame = MiniGame::RESURRECTION;

    inputForceClear();
    clearInputLatch();
    return;
  }

  soundResetDeathDirgeLatch();
  soundFuneralDirge();

  g_app.uiState = UIState::BURIAL_SCREEN;
  requestUIRedraw();

  invalidateBackgroundCache();

  inputForceClear();
  clearInputLatch();
}