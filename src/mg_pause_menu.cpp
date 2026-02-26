#include "mg_pause_menu.h"

#include <Arduino.h>

#include "input.h"
#include "graphics.h"   // spr, screenW/screenH
#include "ui_runtime.h" // colors/text datum constants if needed
#include "display.h"      // screenW, screenH (and often spr depending on your layout)
#include "tft_compat.h"   // TFT_* colors + TC_DATUM / TL_DATUM, etc.

// -----------------------------------------------------------------------------
// Pause state (moved out of mini_games.cpp)
// -----------------------------------------------------------------------------
static bool     s_mgPaused            = false;
static uint8_t  s_mgPauseChoice       = 0;    // 0=Continue, 1=Exit
static bool     s_mgPausePrevSelHeld  = false;

static uint32_t s_mgPauseStartMs      = 0;
static uint32_t s_mgPausedAccumMs     = 0;
static bool     s_mgPrevPaused        = false;
static bool     s_mgJustResumed       = false;

bool mgPauseIsPaused() { return s_mgPaused; }
void mgPauseSetPaused(bool paused) { s_mgPaused = paused; }

uint32_t mgPauseAccumMs() { return s_mgPausedAccumMs; }
uint32_t mgPauseStartMs() { return s_mgPauseStartMs; }

bool mgPauseJustResumedConsume()
{
  const bool v = s_mgJustResumed;
  s_mgJustResumed = false;
  return v;
}

void mgPauseUpdateClocks(uint32_t now)
{
  // Detect transitions and track how long we've been paused total.
  if (s_mgPaused && !s_mgPrevPaused) {
    // Just entered pause
    s_mgPauseStartMs = now;
    s_mgJustResumed  = false;
  }
  else if (!s_mgPaused && s_mgPrevPaused) {
    // Just exited pause
    if (s_mgPauseStartMs != 0) {
      s_mgPausedAccumMs += (now - s_mgPauseStartMs);
    }
    s_mgPauseStartMs = 0;
    s_mgJustResumed  = true;   // one-shot flag; games can consume
  }

  s_mgPrevPaused = s_mgPaused;
}

void mgPauseResetClock()
{
  s_mgPauseStartMs  = 0;
  s_mgPausedAccumMs = 0;
  s_mgPrevPaused    = false;
  s_mgJustResumed   = false;
}

static inline bool mgPauseEnterOnce(const InputState& input)
{
  const bool once = (input.mgSelectHeld && !s_mgPausePrevSelHeld) || input.mgSelectOnce;
  s_mgPausePrevSelHeld = input.mgSelectHeld;
  return once;
}

void mgPauseReset()
{
  s_mgPaused           = false;
  s_mgPauseChoice      = 0;
  s_mgPausePrevSelHeld = false;
  mgPauseResetClock();
}

void mgPauseForceOffNoStick()
{
  // Used when mini-game isn't active-playing; prevent pause sticking
  s_mgPaused       = false;
  s_mgPrevPaused   = false;
  s_mgPauseStartMs = 0;
  // NOTE: keep s_mgPausedAccumMs as-is; it will get reset on next mgPauseReset()
  s_mgJustResumed  = false;
}

uint8_t mgPauseHandle(const InputState& input)
{
  // ESC toggles pause
  if (input.mgQuitOnce) {
    s_mgPaused = !s_mgPaused;
    if (s_mgPaused) s_mgPauseChoice = 0; // default Continue
    s_mgPausePrevSelHeld = input.mgSelectHeld; // prevent instant confirm
    return MGPAUSE_CONSUME;
  }

  if (!s_mgPaused) return MGPAUSE_NONE;

  // Navigate choices
  if (input.mgUpOnce || input.mgLeftOnce) {
    if (s_mgPauseChoice > 0) s_mgPauseChoice--;
  }
  if (input.mgDownOnce || input.mgRightOnce) {
    if (s_mgPauseChoice < 1) s_mgPauseChoice++;
  }

  // Confirm
  if (mgPauseEnterOnce(input)) {
    if (s_mgPauseChoice == 0) {
      s_mgPaused = false;
      s_mgPausePrevSelHeld = input.mgSelectHeld;
      return MGPAUSE_CONSUME;
    } else {
      // Exit selected
      s_mgPaused = false;
      return MGPAUSE_EXIT;
    }
  }

  return MGPAUSE_CONSUME;
}

void mgDrawPauseOverlay()
{
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  // Simple overlay box
  const int bx = 28;
  const int by = 32;
  const int bw = gW - 56;
  const int bh = 72;

  spr.fillRect(bx, by, bw, bh, TFT_BLACK);
  spr.drawRect(bx, by, bw, bh, TFT_DARKGREY);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawCentreString("PAUSED", gW / 2, by + 10, 2);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);

  const int ox = bx + 14;
  const int oy = by + 30;

  spr.drawString((s_mgPauseChoice == 0) ? "> Continue" : "  Continue", ox, oy, 2);
  spr.drawString((s_mgPauseChoice == 1) ? "> Exit"     : "  Exit",     ox, oy + 20, 2);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
  spr.drawCentreString("ESC: Resume   ENTER: Select", gW/2, by + bh - 14, 1);
}