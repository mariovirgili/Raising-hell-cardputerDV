#include "ui_input_common.h"
#include "input.h"

void uiDrainKb(InputState& in)
{
  while (in.kbHasEvent()) (void)in.kbPop();
}

void uiSwallowTypingAndEdges(InputState& in)
{
  uiDrainKb(in);
  in.clearEdges();
  clearInputLatch();
}