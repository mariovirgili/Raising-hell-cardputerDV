#pragma once

/*
  Raising Hell — Project Conventions (Phase 0 Guardrails)

  Keep the codebase clean and predictable while we refactor:

  1) Include order (in every .cpp)
     - Include the module's own header first (e.g., foo.cpp starts with "foo.h")
     - Then Arduino/system headers
     - Then other project headers

  2) Macros
     - Debug macros live ONLY in debug.h (DBG_ON / DBGLN_ON)
     - Build/feature toggles live ONLY in build_flags.h (when added)
     - Do not define macros in random modules or globals.h

  3) Globals / State
     - Do not DEFINE globals in headers. Headers may declare externs, but storage
       belongs in exactly one .cpp.
     - Prefer state modules (*_state.h/.cpp) or AppState (g_app) for shared state.
     - Keep ownership clear: pet save owns pet/inventory; settings save owns user/system toggles.

  4) Dependencies
     - Avoid including heavy headers when a forward declaration will do.
     - Keep gameplay independent of UI where possible (formal boundary work happens later).
*/

#include <Arduino.h>
#include "debug_state.h"
#include <stdint.h>

extern uint32_t g_debugArmMs;

// Set to 0 to completely compile out debug logging
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
  // Serial-pressure-safe debug logging:
  // - gated by runtime g_debugEnabled
  // - drops output if the TX buffer is too full (avoids stalls)
  #define DBG_ON(...) do { \
    if (dbgCanWrite(64)) { Serial.printf(__VA_ARGS__); } \
  } while (0)

  #define DBGLN_ON(s) do { \
    if (dbgCanWrite(24)) { Serial.println(s); } \
  } while (0)
#else
  #define DBG_ON(...)
  #define DBGLN_ON(x)
#endif
