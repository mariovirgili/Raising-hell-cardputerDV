#include "ui_suppress.h"

#include <Arduino.h>

#include "input.h"

// -----------------------------------------------------------------------------
// Internal timers (millis-based, wrap-safe with signed diff)
// -----------------------------------------------------------------------------
static uint32_t s_suppressMenuUntilMs      = 0;
static uint32_t s_suppressSleepWakeUntilMs = 0;

// -----------------------------------------------------------------------------
// Menu / ESC suppression
// -----------------------------------------------------------------------------
void uiSuppressMenuForMs(uint32_t ms)
{
  s_suppressMenuUntilMs = millis() + ms;
}

bool uiIsMenuSuppressed()
{
  return (int32_t)(millis() - s_suppressMenuUntilMs) < 0;
}

uint32_t uiMenuSuppressedUntilMs()
{
  return s_suppressMenuUntilMs;
}

// -----------------------------------------------------------------------------
// Sleep / Wake suppression
// -----------------------------------------------------------------------------
void uiSuppressSleepWakeForMs(uint32_t ms)
{
  s_suppressSleepWakeUntilMs = millis() + ms;
}

bool uiIsSleepWakeSuppressed()
{
  return (int32_t)(millis() - s_suppressSleepWakeUntilMs) < 0;
}

uint32_t uiSleepWakeSuppressedUntilMs()
{
  return s_suppressSleepWakeUntilMs;
}

void uiClearSleepWakeSuppression()
{
  s_suppressSleepWakeUntilMs = 0;
}

// -----------------------------------------------------------------------------
// Transition helper
// -----------------------------------------------------------------------------
void uiGuardTransition(InputState& input, uint32_t menuSuppressMs)
{
  if (menuSuppressMs)
    uiSuppressMenuForMs(menuSuppressMs);

  // Kill any residue input that would double-trigger
  input.clearEdges();
  inputForceClear();
  clearInputLatch();
}