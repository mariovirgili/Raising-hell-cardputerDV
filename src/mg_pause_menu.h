#pragma once
#include <stdint.h>

// Forward declare to avoid heavy includes in the header.
struct InputState;

// Return codes for mgPauseHandle()
enum : uint8_t
{
  MGPAUSE_NONE    = 0,  // nothing special happened
  MGPAUSE_CONSUME = 1,  // pause system consumed input this frame
  MGPAUSE_EXIT    = 2   // user chose "Exit mini-game"
};

// Lifecycle / state
void     mgPauseReset();
void     mgPauseForceOffNoStick();

// Input + timing
uint8_t  mgPauseHandle(const InputState& input);
void     mgPauseUpdateClocks(uint32_t nowMs);

// Query helpers
bool     mgPauseIsPaused();
uint32_t mgPauseStartMs();
uint32_t mgPauseAccumMs();
bool     mgPauseJustResumedConsume();

// Direct setters (used by UI-state wrappers)
void     mgPauseSetPaused(bool paused);
void     mgPauseSetChoice(uint8_t choice);
uint8_t  mgPauseChoice();

// Draw
void     mgDrawPauseOverlay();