#include "mg_pause_core.h"
#include "input.h"
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pause core state
// -----------------------------------------------------------------------------

static bool     s_paused       = false;
static uint32_t s_pauseStartMs = 0;
static uint32_t s_pauseAccumMs = 0;   // total time spent paused
static bool     s_justResumed  = false;

// Edge tracking (so holding ESC/ENTER doesn’t toggle repeatedly)
static bool s_prevEscHeld   = false;
static bool s_prevEnterHeld = false;

// Overlay selection (0 = Continue, 1 = Exit)
static uint8_t s_choice = 0;

// If you already have constants elsewhere, match them.
static const uint8_t MGPAUSE_CONTINUE = 0;
static const uint8_t MGPAUSE_EXIT     = 1;

void mgPauseReset()
{
  s_paused        = false;
  s_pauseStartMs  = 0;
  s_pauseAccumMs  = 0;
  s_justResumed   = false;
  s_prevEscHeld   = false;
  s_prevEnterHeld = false;
  s_choice        = 0;
}

bool mgPauseIsPaused()
{
  return s_paused;
}

void mgPauseForceOffNoStick()
{
  s_paused       = false;
  s_pauseStartMs = 0;
  // Don’t touch accum; it’s harmless, but clear edge + inputs
  s_justResumed   = false;
  s_prevEscHeld   = false;
  s_prevEnterHeld = false;
  s_choice        = 0;
}

static inline bool escPressedEdge(const InputState& in)
{
  // In this project, ESC maps to mgQuit* (see input.h)
  // Treat mgQuitOnce as an edge, and also synthesize an edge from held.
  const bool held = in.mgQuitHeld;
  const bool edge = in.mgQuitOnce || (held && !s_prevEscHeld);
  s_prevEscHeld = held;
  return edge;
}

static inline bool enterPressedEdge(const InputState& in)
{
  // In this project, ENTER maps to mgSelect* (see input.h)
  const bool held = in.mgSelectHeld;
  const bool edge = in.mgSelectOnce || (held && !s_prevEnterHeld);
  s_prevEnterHeld = held;
  return edge;
}

static void enterPause(uint32_t nowMs)
{
  s_paused       = true;
  s_pauseStartMs = nowMs;
  s_choice       = 0;        // default highlight “Continue”
  s_justResumed  = false;
}

static void leavePause(uint32_t nowMs)
{
  if (s_paused)
  {
    // accumulate time spent paused
    const uint32_t dt = nowMs - s_pauseStartMs;
    s_pauseAccumMs += dt;
  }

  s_paused       = false;
  s_pauseStartMs = 0;
  s_justResumed  = true;
}

void mgPauseUpdateClocks(uint32_t nowMs)
{
  // This exists so callers can “tick” pause timing every frame if desired.
  // Nothing to do here right now.
  (void)nowMs;
}

uint32_t mgPauseAccumulatedMs()
{
  return s_pauseAccumMs;
}

uint32_t mgPauseElapsedSince(uint32_t startMs, uint32_t nowMs)
{
  // elapsed excluding paused time.
  // If currently paused, don’t count time since pause began either.
  uint32_t pausedTotal = s_pauseAccumMs;
  if (s_paused) {
    pausedTotal += (nowMs - s_pauseStartMs);
  }

  const uint32_t raw = (nowMs >= startMs) ? (nowMs - startMs) : 0;
  return (raw >= pausedTotal) ? (raw - pausedTotal) : 0;
}

bool mgPauseConsumeJustResumed()
{
  const bool v = s_justResumed;
  s_justResumed = false;
  return v;
}

MgPauseGateResult mgPauseGate(const InputState& input, uint32_t nowMs, bool activePlay)
{
  // If the mini-game isn’t in “active play” (reward modal, etc), don’t allow pausing to stick.
  if (!activePlay)
  {
    if (s_paused) mgPauseForceOffNoStick();
    return MG_GATE_RUN;
  }

  // ESC toggles pause/resume
  if (escPressedEdge(input))
  {
    if (!s_paused) enterPause(nowMs);
    else leavePause(nowMs);

    return s_paused ? MG_GATE_SKIP : MG_GATE_RUN;
  }

  if (!s_paused)
  {
    return MG_GATE_RUN;
  }

  // While paused: UP/DOWN moves choice, ENTER selects
  // Project input uses mgUp*/mgDown* (and encoderDelta).
  if (input.mgUpOnce || input.mgUpHeld || (input.encoderDelta < 0))   s_choice = MGPAUSE_CONTINUE;
  if (input.mgDownOnce || input.mgDownHeld || (input.encoderDelta > 0)) s_choice = MGPAUSE_EXIT;

  if (enterPressedEdge(input))
  {
    if (s_choice == MGPAUSE_EXIT)
    {
      leavePause(nowMs); // resume clock first so callers don’t inherit paused offsets
      return MG_GATE_EXIT;
    }

    leavePause(nowMs);
    return MG_GATE_RUN;
  }

  return MG_GATE_SKIP;
}