#include "mg_pause_core.h"

#include <Arduino.h>

#include "mg_pause_menu.h" // MGPAUSE_* constants
#include "input.h"

// -----------------------------------------------------------------------------
// Single source of truth for pause state
// -----------------------------------------------------------------------------

static bool     s_paused = false;
static uint32_t s_pauseStartMs = 0;
static uint32_t s_pauseAccumMs = 0;
static bool     s_justResumed = false;

// Menu selection (0 = Continue, 1 = Exit)
static uint8_t  s_choice = 0;

static inline void setPausedInternal(bool p, uint32_t now)
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
    if (s_pauseStartMs != 0)
    {
      s_pauseAccumMs += (now - s_pauseStartMs);
      s_pauseStartMs = 0;
    }
    s_paused = false;
    s_justResumed = true;
  }
}

// -----------------------------------------------------------------------------
// Gate API (existing)
// -----------------------------------------------------------------------------

void mgPauseGateReset()
{
  s_paused = false;
  s_pauseStartMs = 0;
  s_pauseAccumMs = 0;
  s_justResumed = false;
  s_choice = 0;
}

bool mgPauseGateIsPaused() { return s_paused; }

uint32_t mgPauseGateStartMs() { return s_pauseStartMs; }

uint32_t mgPauseGateAccumulatedMs() { return s_pauseAccumMs; }

bool mgPauseGateConsumeJustResumed()
{
  if (!s_justResumed) return false;
  s_justResumed = false;
  return true;
}

void mgPauseGateSetPaused(bool paused)
{
  setPausedInternal(paused, millis());
}

void mgPauseGateUpdateClocks(uint32_t nowMs, bool activePlay, uint32_t &io_activePlayMs)
{
  // If not paused, time always advances normally.
  if (!s_paused)
  {
    io_activePlayMs = nowMs;
    return;
  }

  // If paused:
  // - while actively playing, freeze activePlayMs at pause start
  // - when not actively playing (e.g., reward modal), let it advance
  if (activePlay)
  {
    if (s_pauseStartMs == 0) s_pauseStartMs = nowMs;
    io_activePlayMs = s_pauseStartMs;
  }
  else
  {
    io_activePlayMs = nowMs;
  }
}

MgPauseGateResult mgPauseGateHandle(const InputState& input)
{
  // If we aren't paused, game runs normally.
  if (!s_paused)
  {
    return MgPauseGateResult::MG_GATE_RUN;
  }

  // If paused, let the pause menu consume input / update selection.
  // mgPauseHandle() returns MGPAUSE_* constants.
  const uint8_t r = mgPauseHandle(input);

  // If pause menu says exit, bubble that up to the minigame runner.
  if (r == MGPAUSE_EXIT)
  {
    // Usually you want to clear paused state when exiting.
    setPausedInternal(false, millis());
    return MgPauseGateResult::MG_GATE_EXIT;
  }

  // If pause menu says resume, clear pause and allow game to run.
  if (r == MGPAUSE_NONE)
  {
    // If your mgPauseHandle() uses NONE to mean "not paused / resume",
    // ensure gate state matches.
    if (s_paused) setPausedInternal(false, millis());
    return MgPauseGateResult::MG_GATE_RUN;
  }

  // Otherwise we remain paused; game update should be skipped.
  return MgPauseGateResult::MG_GATE_SKIP;
}

// -----------------------------------------------------------------------------
// Public "classic" API used by mini_games.cpp
// -----------------------------------------------------------------------------

void mgPauseReset() { mgPauseGateReset(); }

bool mgPauseIsPaused() { return mgPauseGateIsPaused(); }

void mgPauseSetPaused(bool paused) { mgPauseGateSetPaused(paused); }

uint32_t mgPauseStartMs() { return mgPauseGateStartMs(); }

uint32_t mgPauseAccumMs() { return mgPauseGateAccumulatedMs(); }

bool mgPauseJustResumedConsume() { return mgPauseGateConsumeJustResumed(); }

void mgPauseUpdateClocks(uint32_t nowMs, bool activePlay, uint32_t &io_activePlayMs)
{
  mgPauseGateUpdateClocks(nowMs, activePlay, io_activePlayMs);
}

// -----------------------------------------------------------------------------
// Legacy compatibility wrapper (used by mini_games.cpp etc.)
// -----------------------------------------------------------------------------

void mgPauseUpdateClocks(uint32_t nowMs)
{
  // If not paused, nothing special to do.
  if (!s_paused)
    return;

  // While paused, accumulate time.
  if (s_pauseStartMs == 0)
    s_pauseStartMs = nowMs;
}

void mgPauseSetChoice(uint8_t choice) { s_choice = (choice != 0) ? 1 : 0; }

uint8_t mgPauseChoice() { return s_choice; }

// IMPORTANT: mgPauseHandle is the menu-layer input handler and should return MGPAUSE_*.
// If you have a real implementation elsewhere (mg_pause_menu.cpp), delete this stub
// and link against the real one instead.
uint8_t mgPauseHandle(const InputState& input)
{
  const uint32_t now = millis();

  // While not paused, only ESC (mgQuitOnce) can enter pause (handled elsewhere).
  // This function primarily handles navigation/selection *while paused*.
  if (!s_paused) return MGPAUSE_NONE;

  // ESC resumes immediately.
  if (input.mgQuitOnce)
  {
    setPausedInternal(false, now);
    return MGPAUSE_CONSUME;
  }

  // Navigate options (0 = Continue, 1 = Exit).
  if (input.mgUpOnce || input.mgDownOnce)
  {
    s_choice = (s_choice == 0) ? 1 : 0;
    return MGPAUSE_CONSUME;
  }

  // ENTER selects.
  if (input.mgSelectOnce)
  {
    if (s_choice == 0)
    {
      setPausedInternal(false, now);
      return MGPAUSE_CONSUME;
    }
    else
    {
      setPausedInternal(false, now);
      return MGPAUSE_EXIT;
    }
  }

  return MGPAUSE_CONSUME;
}