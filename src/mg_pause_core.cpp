#include "mg_pause_core.h"

#include <Arduino.h>

#include "mg_pause_menu.h" // MGPAUSE_* constants
#include "input.h"
#include "ui_mg_pause_menu.h" // uiMgPauseMenuActivate(), uiMgPauseMenuCount()

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
  const uint32_t now = millis();

  // ESC (mapped to mgQuitOnce) toggles pause.
  if (input.mgQuitOnce)
  {
    if (!s_paused)
    {
      setPausedInternal(true, now);
      return MgPauseGateResult::MG_GATE_SKIP; // paused now; skip game update
    }

    // If already paused, ESC resumes.
    setPausedInternal(false, now);
    return MgPauseGateResult::MG_GATE_RUN;
  }

  // Not paused? game runs normally.
  if (!s_paused)
  {
    return MgPauseGateResult::MG_GATE_RUN;
  }

  // Paused: consume input and possibly resume/exit.
  const uint8_t r = mgPauseHandle(input);

  if (r == MGPAUSE_EXIT)
  {
    return MgPauseGateResult::MG_GATE_EXIT;
  }

  // If Continue was chosen, mgPauseSetPaused(false) will have cleared s_paused.
  if (!s_paused)
  {
    return MgPauseGateResult::MG_GATE_RUN;
  }

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

void mgPauseForceOffNoStick()
{
  // Hard stop: unpause without caring about “stickiness”
  setPausedInternal(false, millis());
}

void mgPauseSetChoice(uint8_t choice)
{
  s_choice = (choice != 0) ? 1 : 0;
}

uint8_t mgPauseChoice()
{
  return s_choice;
}

// -----------------------------------------------------------------------------
// Legacy compatibility wrapper (used by mini_games.cpp etc.)
// -----------------------------------------------------------------------------

void mgPauseUpdateClocks(uint32_t nowMs)
{
  // This overload exists because older code calls mgPauseUpdateClocks(nowMs).
  // We only need it to "freeze" pause start while paused.
  if (!s_paused)
  {
    return;
  }

  if (s_pauseStartMs == 0)
  {
    s_pauseStartMs = nowMs;
  }
}

uint8_t mgPauseHandle(const InputState& input)
{
  if (!s_paused)
  {
    return MGPAUSE_NONE;
  }

  const int count = uiMgPauseMenuCount();
  if (count > 0 && (int)s_choice >= count)
  {
    s_choice = 0;
  }

  // Up/Down changes selection
  if (input.mgUpOnce || input.mgDownOnce)
  {
    if (count <= 1)
    {
      s_choice = 0;
    }
    else
    {
      // 2-item menu: just toggle
      s_choice = (s_choice == 0) ? 1 : 0;
    }
    return MGPAUSE_NONE;
  }

  // Enter activates selection
  if (input.mgSelectOnce)
  {
    // ui action handlers expect a non-const InputState&
    InputState& in = const_cast<InputState&>(input);
    uiMgPauseMenuActivate((int)s_choice, in);

    // If “Continue” was chosen, it should have unpaused us.
    if (!s_paused)
    {
      return MGPAUSE_NONE;
    }

    // If “Exit” was chosen, the UI action should have queued return UI.
    // We still return EXIT so the mini-game runner can bail immediately.
    if (s_choice == 1)
    {
      return MGPAUSE_EXIT;
    }

    return MGPAUSE_NONE;
  }

  return MGPAUSE_NONE;
}