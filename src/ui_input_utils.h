#pragma once

#include "input.h"

// ---------------------------------------------------------------------------
// Navigation helpers — used by every per-screen input handler
// ---------------------------------------------------------------------------

// Returns -1, 0, or +1 from encoder + up/down keys.
static inline int uiNavMove(const InputState& in)
{
  if (in.upOnce   || in.encoderDelta < 0) return -1;
  if (in.downOnce || in.encoderDelta > 0) return +1;
  return 0;
}

// Returns true if the "select / confirm" action fired this frame.
static inline bool uiIsSelect(const InputState& in)
{
  return in.selectOnce || in.encoderPressOnce;
}

// Returns true if the "back / cancel" action fired this frame.
static inline bool uiIsBack(const InputState& in)
{
  return in.menuOnce || in.escOnce;
}

// Applies move to idx with wrap-around in [0, count).
static inline void uiWrapIndex(int& idx, int move, int count)
{
  if (count <= 0) return;
  idx = (idx + move % count + count) % count;
}