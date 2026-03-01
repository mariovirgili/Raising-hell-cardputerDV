#include "ui_state_mini_game.h"

#include <Arduino.h>

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "mg_pause_core.h"
#include "mg_pause_menu.h"
#include "mini_games.h"
#include "ui_actions.h"
#include "ui_runtime.h"

void uiMiniGameHandle(InputState& in)
{
  if (!g_app.inMiniGame)
  {
    uiActionEnterStateClean(UIState::PET_SCREEN, Tab::TAB_PET, false, in, 150);
    requestFullUIRedraw();
    return;
  }

  // Pause-gate handles ESC toggle and pause menu interaction.
  const MgPauseGateResult gate = mgPauseGateHandle(in);

  if (gate == MgPauseGateResult::MG_GATE_EXIT)
  {
    // Exit immediately back to return UI.
    miniGameExitToReturnUi(true);
    requestFullUIRedraw();
    return;
  }

  if (gate == MgPauseGateResult::MG_GATE_SKIP)
  {
    // paused: only draw overlay/menu elsewhere
    return;
  }

  // Running: update/draw mini game
  updateMiniGame(in);
  drawMiniGame();

  // If paused got activated during update, switch to MG_PAUSE UI state.
  if (mgPauseIsPaused())
  {
    uiActionEnterStateClean(UIState::MG_PAUSE, g_app.currentTab, false, in, 150);
    requestFullUIRedraw();
  }
}