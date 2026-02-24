#pragma once

#include "input.h"

// Swallow any pending key events / pulses for the current frame.
// This mirrors the local drainKb(...) helpers you had in several files.
static inline void uiDrainKb(InputState& in) {
  // If your InputState has more fields you want to clear, add them here.
  // We keep it conservative and only clear what those local helpers cleared.

  in.upOnce = false;
  in.downOnce = false;
  in.leftOnce = false;
  in.rightOnce = false;
  in.menuOnce = false;
  in.escOnce = false;
  in.selectOnce = false;
  in.encoderPressOnce = false;

  in.encoderDelta = 0;

  // If you have other “once” flags (A/B buttons etc), add them too.
}