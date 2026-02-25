#include "ui_state_evolution.h"

#include "input.h"

void uiEvolutionHandle(InputState& in)
{
  while (in.kbHasEvent())
    (void)in.kbPop();

  in.clearEdges();
}