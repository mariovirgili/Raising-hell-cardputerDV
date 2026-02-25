#include "ui_actions.h"

#include "app_state.h"
#include "input.h"
#include "ui_runtime.h"
#include "graphics.h"    
#include "display.h" 
#include "ui_input_common.h"
#include "ui_state_lifecycle.h"

void uiActionRequestRedraw()
{
  requestUIRedraw();
}

void uiActionRequestFullRedraw()
{
  requestFullUIRedraw();
}

void uiActionDrainKb(InputState& in)
{
  uiDrainKb(in);
}

void uiActionSwallowEdges(InputState& in)
{
  uiActionDrainKb(in);
  in.clearEdges();
}

void uiActionSwallowAll(InputState& in)
{
  uiActionSwallowEdges(in);
  inputForceClear();
  clearInputLatch();
}

void uiActionEnterState(UIState state, Tab tab, bool redraw)
{
  const UIState old = g_app.uiState;

  if (old != state)
    uiStateOnExit(old);

  g_app.uiState = state;
  g_app.currentTab = tab;

  if (old != state)
    uiStateOnEnter(state);

  if (redraw)
    requestUIRedraw();
}