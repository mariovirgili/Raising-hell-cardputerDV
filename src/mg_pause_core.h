#pragma once
#include <stdint.h>

struct InputState;

// Results for a “pause gate” check.
enum MgPauseGateResult : uint8_t {
  MG_GATE_RUN = 0,   // Run game update/draw normally
  MG_GATE_SKIP,      // Don’t advance game simulation this frame (paused)
  MG_GATE_EXIT        // User chose Exit from pause overlay
};

// Core pause lifecycle
void mgPauseReset();
bool mgPauseIsPaused();

// Pause gating + input consumption
MgPauseGateResult mgPauseGate(const InputState& input, uint32_t nowMs, bool activePlay);

// Timekeeping helpers
void mgPauseUpdateClocks(uint32_t nowMs);
uint32_t mgPauseAccumulatedMs();                 // total paused time so far
uint32_t mgPauseElapsedSince(uint32_t startMs, uint32_t nowMs);  // elapsed excluding paused time

// Edges
bool mgPauseConsumeJustResumed();                // true once after resume

// Safety: force off without leaving “sticky” state
void mgPauseForceOffNoStick();