#include "ui_state_death.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "mini_games.h"
#include "sound.h"
#include "ui_invalidate.h"
#include "ui_state_utils.h" 
#include "game_options_state.h" 
#include "death_state.h"

extern void startResurrectionRun();
extern void soundResetDeathDirgeLatch();
extern void soundFuneralDirge();

void uiDeathHandle(InputState& in) {
  int move = 0;
  if (in.upOnce) move = -1;
  if (in.downOnce) move = +1;
  if (in.encoderDelta < 0) move = -1;
  if (in.encoderDelta > 0) move = +1;

  if (move != 0) {
    deathMenuIndex += (move > 0) ? 1 : -1;
    if (deathMenuIndex < 0) deathMenuIndex = 1;
    if (deathMenuIndex > 1) deathMenuIndex = 0;

    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (!(in.selectOnce || in.encoderPressOnce)) return;

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