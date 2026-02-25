#include "ui_actions.h"

#include "app_state.h"
#include "input.h"
#include "ui_runtime.h"
#include "graphics.h"    // invalidateBackgroundCache()
#include "display.h"     // requestFullUIRedraw()

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
  while (in.kbHasEvent())
    (void)in.kbPop();
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
  g_app.uiState = state;
  g_app.currentTab = tab;
  if (redraw)
    requestUIRedraw();
}