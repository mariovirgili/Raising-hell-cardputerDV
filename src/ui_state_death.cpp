#include "ui_state_death.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "mini_games.h"
#include "sound.h"
#include "ui_input_common.h"
#include "ui_input_utils.h"
#include "ui_runtime.h"
#include "ui_death_menu.h"

extern int deathMenuIndex;

void uiDeathHandle(InputState& in)
{
  const int move = uiNavMove(in);
  if (move != 0) {
    uiWrapIndex(deathMenuIndex, move, uiDeathMenuCount());
    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (!uiIsSelect(in)) return;

  uiDeathMenuActivate(deathMenuIndex, in);
}