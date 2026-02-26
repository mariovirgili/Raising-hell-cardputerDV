#include "mg_pause_menu.h"

#include <Arduino.h>

#include "input.h"
#include "display.h"      // spr, screenW/screenH, TFT_* colors, *_DATUM
#include "graphics.h"
#include "ui_runtime.h"
#include "mg_pause.h"   // or whatever header declares mgPause* + mgDrawPauseOverlay

// -----------------------------------------------------------------------------
// Minimal pause manager implementation
// (Compiles now; you can wire actual ESC/ENTER semantics inside mgPauseHandle())
// -----------------------------------------------------------------------------

static bool     s_paused = false;
static uint32_t s_pauseStartMs = 0;
static uint32_t s_pauseAccumMs = 0;
static bool     s_justResumed = false;

// Menu selection (0 = Continue, 1 = Exit)
static uint8_t  s_choice = 0;

static inline void setPaused(bool p, uint32_t now)
{
  if (p == s_paused) return;

  if (p)
  {
    s_paused = true;
    s_pauseStartMs = now;
    s_choice = 0;
  }
  else
  {
    // leaving pause: accumulate time
    if (s_pauseStartMs != 0)
    {
      s_pauseAccumMs += (now - s_pauseStartMs);
      s_pauseStartMs = 0;
    }
    s_paused = false;
    s_justResumed = true;
  }
}

uint32_t mgPauseStartMs() { return s_pauseStartMs; }

uint32_t mgPauseAccumMs()
{
  // Only the accumulated finished pauses (active pause time is handled by callers).
  return s_pauseAccumMs;
}

bool mgPauseJustResumedConsume()
{
  if (!s_justResumed) return false;
  s_justResumed = false;
  return true;
}

void mgPauseSetPaused(bool paused)
{
  // Use current time for correct clock accumulation.
  const uint32_t now = millis();
  setPaused(paused, now);
}

void mgPauseSetChoice(uint8_t choice)
{
  s_choice = (choice != 0) ? 1 : 0;
}

uint8_t mgPauseChoice()
{
  return s_choice;
}

uint8_t mgPauseHandle(const InputState& input)
{
  const uint32_t now = millis();
  (void)input;

  // ---------------------------------------------------------------------------
  // IMPORTANT:
  // Right now we *do not* assume any specific InputState members exist for ESC/ENTER
  // (because your build errors show older names like escPressed/enterPressed were removed).
  //
  // This function is the correct place to map your real keys once you confirm the
  // actual InputState fields (ex: input.escOnce, input.keyEscOnce, etc).
  //
  // For now: no toggling, no exit. This makes the build clean immediately.
  // ---------------------------------------------------------------------------

  if (!s_paused) return MGPAUSE_NONE;

  // If we ever setPaused(true), we'd handle menu navigation here using known-safe inputs.
  // Keep it conservative for now.
  (void)now;
  return MGPAUSE_CONSUME;
}

void mgDrawPauseOverlay()
{
  // Minimal overlay (safe even if you keep pause disabled for the moment)
  if (!s_paused) return;

  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  const int bw = gW - 40;
  const int bh = 70;
  const int bx = 20;
  const int by = (gH - bh) / 2;

  spr.fillRect(bx, by, bw, bh, TFT_BLACK);
  spr.drawRect(bx, by, bw, bh, TFT_DARKGREY);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawCentreString("PAUSED", gW / 2, by + 8, 2);

  spr.setTextDatum(TL_DATUM);
  const int ox = bx + 12;
  const int oy = by + 28;

  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString((s_choice == 0) ? "> Continue" : "  Continue", ox, oy, 2);
  spr.drawString((s_choice == 1) ? "> Exit"     : "  Exit",     ox, oy + 20, 2);

  spr.setTextDatum(BC_DATUM);
  spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  spr.drawCentreString("ESC: Resume   ENTER: Select", gW/2, by + bh - 8, 1);
}