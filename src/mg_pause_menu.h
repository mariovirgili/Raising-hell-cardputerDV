#pragma once

#include <stdint.h>

struct InputState;

// Return codes for pause handler
static const uint8_t MGPAUSE_NONE    = 0;
static const uint8_t MGPAUSE_CONSUME = 1; // paused UI consumed tick
static const uint8_t MGPAUSE_EXIT    = 2; // user chose Exit

// Pause state
bool mgPauseIsPaused();
void mgPauseSetPaused(bool paused);

// Pause menu input + drawing
uint8_t mgPauseHandle(const InputState& input);
void mgDrawPauseOverlay();

// Pause clocking (so games can subtract paused time)
void mgPauseUpdateClocks(uint32_t now);
void mgPauseReset();            // reset pause + choice + clocks (new game start)
void mgPauseResetClock();       // reset only clocks
uint32_t mgPauseAccumMs();       // completed paused segments total
uint32_t mgPauseStartMs();       // current pause segment start (0 if not in-segment)
bool mgPauseJustResumedConsume(); // one-shot: true once right after resume

// Utility used in mini_games.cpp to prevent pause “sticking” when not active-playing.
// Leaves accumulated time intact (will be reset on next mgPauseReset()).
void mgPauseForceOffNoStick();