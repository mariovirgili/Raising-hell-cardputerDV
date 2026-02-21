#include "power_button.h"

#include <Arduino.h>
#include "ui_invalidate.h"
#include "input.h"          // inputForceClear(), clearInputLatch()
#include "display.h"        // isScreenOn(), toggleScreenPower(), requestUIRedraw()
#include "menu_actions.h"   // openPowerMenuFromHere(now)
#include "input_activity_state.h" // setLastInputActivityMs(now)

// --- GO button (Cardputer spec: GPIO0) ---
static constexpr int  GO_BTN_PIN    = 0;
static constexpr bool GO_ACTIVE_LOW = true;

static inline bool readGoRaw() {
  return GO_ACTIVE_LOW ? (digitalRead(GO_BTN_PIN) == LOW)
                       : (digitalRead(GO_BTN_PIN) == HIGH);
}

void powerButtonInit() {
  pinMode(GO_BTN_PIN, INPUT_PULLUP);
}

void powerButtonTick(uint32_t now) {
  // Tunables (asymmetric debounce: quick presses register, releases are filtered)
  const uint32_t kDebounceDownMs  = 8;    // quick tap friendly
  const uint32_t kDebounceUpMs    = 18;   // filter release chatter
  const uint32_t kMinPressMs      = 10;   // shorter presses should still toggle
  const uint32_t kLongPressMs     = 650;  // hold to open power menu
  const uint32_t kToggleLockoutMs = 160;  // reduce “ignored second press” feeling

  // State
  static bool     s_inited        = false;
  static bool     s_lastRaw       = false;
  static uint32_t s_lastChangeMs  = 0;

  static bool     s_pressed       = false;
  static uint32_t s_pressStartMs  = 0;

  static uint32_t s_lastToggleMs  = 0;
  static bool     s_longFired     = false;

  const bool raw = readGoRaw();

  // Seed state (DON’T early-return in a way that can swallow the first user press)
  if (!s_inited) {
    s_inited       = true;
    s_lastRaw      = raw;
    s_lastChangeMs = now;
    s_pressed      = raw;
    s_pressStartMs = now;
    s_longFired    = false;
    return; // baseline; next loop handles real edges
  }

  // Track changes for debounce timing
  if (raw != s_lastRaw) {
    s_lastRaw = raw;
    s_lastChangeMs = now;
  }

  // Wait until stable (asymmetric)
  const uint32_t needStableMs = s_lastRaw ? kDebounceDownMs : kDebounceUpMs;
  if (now - s_lastChangeMs < needStableMs) return;

  const bool stableDown = s_lastRaw;

  // Press edge (stable)
  if (stableDown && !s_pressed) {
    s_pressed = true;
    s_pressStartMs = now;
    s_longFired = false;
  }

  // While held: long press fires once (open power menu)
  if (s_pressed && stableDown && !s_longFired) {
    if (now - s_pressStartMs >= kLongPressMs) {
      s_longFired = true;

      // Long-press: open Power Menu (reboot/shutdown).
      // openPowerMenuFromHere() already snapshots return state, wakes screen if needed,
      // switches UIState, redraws, and clears input latches.
      openPowerMenuFromHere(now);
      return;
    }
  }

  // Release edge (stable)
  if (!stableDown && s_pressed) {

    const uint32_t heldMs = now - s_pressStartMs;
    s_pressed = false;

    // If long press fired, we’re done on release.
    if (s_longFired) return;

    // Short press sanity
    if (heldMs < kMinPressMs) return;

    // Lockout prevents bounce causing a second toggle right after the first
    if (now - s_lastToggleMs < kToggleLockoutMs) return;

    s_lastToggleMs = now;

    const bool wasOn = isScreenOn();
    toggleScreenPower();

    // If we just woke the screen, make sure we don’t instantly auto-sleep
    // and that UI/input is in a good state.
    if (!wasOn && isScreenOn()) {
      setLastInputActivityMs(now);  // prevents “wake then immediately sleep”
      requestUIRedraw();
      inputForceClear();
      clearInputLatch();
    }
  }
}
