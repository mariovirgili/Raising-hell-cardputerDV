#pragma once
#include <stdint.h>

// Canonical storage for last user activity timestamp (ms since boot).
// Keep this extern for compatibility with any remaining legacy code.
extern uint32_t g_lastInputActivityMs;

// Preferred accessors (what your .ino is calling now)
uint32_t getLastInputActivityMs();
void     setLastInputActivityMs(uint32_t ms);

struct InputState;
bool hasUserActivity(const InputState& in);
