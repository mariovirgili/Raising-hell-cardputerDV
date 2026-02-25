#include "ui_state_hatching.h"

#include "input.h"

// HATCHING is an automatic flow state.
// It should not react to random buffered input.
// We explicitly drain keyboard events and clear edge flags.
void uiHatchingHandle(InputState& in)
{
  while (in.kbHasEvent())
    (void)in.kbPop();

  in.clearEdges();
}