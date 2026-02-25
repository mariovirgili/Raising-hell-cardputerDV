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

// -----------------------------------------------------------------------------
// Unified semantic input helpers
// -----------------------------------------------------------------------------
int uiNavMove(const InputState& in)
{
  int move = in.encoderDelta;
  if (in.upOnce) move = -1;
  if (in.downOnce) move = 1;
  return move;
}

bool uiSelectPressed(const InputState& in)
{
  return (in.selectOnce || in.encoderPressOnce);
}

bool uiBackPressed(const InputState& in)
{
  return (in.menuOnce || in.escOnce);
}

void uiWrapIndex(int& idx, int count)
{
  if (count <= 0) return;
  if (idx < 0) idx = count - 1;
  if (idx >= count) idx = 0;
}