#pragma once
#include <stdint.h>

// Forward decl so headers don't balloon.
struct InputState;

// -----------------------------------------------------------------------------
// Menu / ESC suppression
// -----------------------------------------------------------------------------
void uiSuppressMenuForMs(uint32_t ms);
bool uiIsMenuSuppressed();
uint32_t uiMenuSuppressedUntilMs();

// -----------------------------------------------------------------------------
// Sleep / Wake suppression
// -----------------------------------------------------------------------------
void uiSuppressSleepWakeForMs(uint32_t ms);
bool uiIsSleepWakeSuppressed();
uint32_t uiSleepWakeSuppressedUntilMs();
void uiClearSleepWakeSuppression();

// -----------------------------------------------------------------------------
// Transition helper
// -----------------------------------------------------------------------------
// Use when switching UI states to avoid accidental double-presses.
// - Optionally suppress MENU/ESC for a short time
// - Forces a full UI redraw (cache invalidation)
// - Clears one-shot edges and the input latch
void uiGuardTransition(InputState& input, uint32_t menuSuppressMs = 0);