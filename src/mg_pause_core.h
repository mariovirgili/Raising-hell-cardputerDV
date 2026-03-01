#pragma once

#include <stdint.h>

struct InputState;

// Result from the pause gate: what the mini-game runner should do this frame.
enum class MgPauseGateResult : uint8_t
{
  MG_GATE_RUN  = 0,  // run normal update/draw
  MG_GATE_SKIP = 1,  // paused: skip updating game simulation
  MG_GATE_EXIT = 2,  // paused+Exit chosen: bail to return UI
};

// -----------------------------------------------------------------------------
// Gate API (pause truth + timing freeze)
// -----------------------------------------------------------------------------
void     mgPauseGateReset();
bool     mgPauseGateIsPaused();
uint32_t mgPauseGateStartMs();
uint32_t mgPauseGateAccumulatedMs();
bool     mgPauseGateConsumeJustResumed();
void     mgPauseGateSetPaused(bool paused);

// Freeze/unfreeze a caller-owned “active play clock”
void mgPauseGateUpdateClocks(uint32_t nowMs, bool activePlay, uint32_t& io_activePlayMs);

// Handle ESC toggle + paused-menu input consumption.
MgPauseGateResult mgPauseGateHandle(const InputState& input);

// -----------------------------------------------------------------------------
// Public / legacy API used by mini_games.cpp (thin wrappers)
// -----------------------------------------------------------------------------
void     mgPauseReset();
bool     mgPauseIsPaused();
void     mgPauseSetPaused(bool paused);
uint32_t mgPauseStartMs();
uint32_t mgPauseAccumMs();
bool     mgPauseJustResumedConsume();

// New-style clock update (preferred)
void mgPauseUpdateClocks(uint32_t nowMs, bool activePlay, uint32_t& io_activePlayMs);

// Legacy single-arg wrapper (keep only if existing callers still use it)
void mgPauseUpdateClocks(uint32_t nowMs);

// Menu selection helpers (0=Continue, 1=Exit)
void     mgPauseSetChoice(uint8_t choice);
uint8_t  mgPauseChoice();

// Pause menu handler return values are MGPAUSE_* (declared in mg_pause_menu.h)
uint8_t mgPauseHandle(const InputState& input);

// Hard unpause
void mgPauseForceOffNoStick();