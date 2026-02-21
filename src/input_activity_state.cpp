#include "input_activity_state.h"
#include "input.h"

// If you already define this somewhere else, DELETE one of the definitions.
// (But based on your current linker error, the getter is missing, not this.)
uint32_t g_lastInputActivityMs = 0;

uint32_t getLastInputActivityMs() {
  return g_lastInputActivityMs;
}

void setLastInputActivityMs(uint32_t ms) {
  g_lastInputActivityMs = ms;
}

// Conservative activity heuristic: any "Once" edge counts as activity.
bool hasUserActivity(const InputState& in) {
  // "Held" is reliable on Cardputer; count any held state as activity.
  return in.menuHeld || in.selectHeld;
}
