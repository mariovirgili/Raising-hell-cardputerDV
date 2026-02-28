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
void uiGuardTransition(InputState& input, uint32_t menuSuppressMs = 0);