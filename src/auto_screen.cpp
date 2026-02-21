#include "auto_screen.h"

#include <Arduino.h>

#include "input_activity_state.h"
#include "display.h"
#include "ui_invalidate.h"
#include "input.h"
#include "brightness_state.h"
#include "user_toggles_state.h"

uint8_t autoScreenTimeoutSel = 0;

bool g_screenIsOff = false;

static bool s_autoScreenOffEnabled = true;

bool autoScreenIsEnabled() {
  return s_autoScreenOffEnabled;
}

void autoScreenSetEnabled(bool enabled) {
  s_autoScreenOffEnabled = enabled;
}

const char* autoScreenToText(uint8_t sel) {
  switch (sel) {
    case 0: return "5m";
    case 1: return "30m";
    case 2: return "1h";
    default: return "Off";
  }
}

uint32_t autoScreenTimeoutMsForSel(uint8_t sel) {
  switch (sel) {
    case 0: return  5UL * 60UL * 1000UL;
    case 1: return 30UL * 60UL * 1000UL;
    case 2: return 60UL * 60UL * 1000UL;
    default: return 0; // Off
  }
}

uint32_t autoScreenTimeoutMs() {
  return autoScreenTimeoutMsForSel((uint8_t)autoScreenTimeoutSel);
}

void screenSleep() {
  if (!isScreenOn()) return;
  SET_SCREEN_POWER(false);
}

void screenWake() {
  if (isScreenOn()) return;
  SET_SCREEN_POWER(true);
  requestUIRedraw();
  inputForceClear();
  clearInputLatch();
}

void noteUserActivity() {
setLastInputActivityMs(millis());

  // If we're asleep/off, wake immediately.
  if (!isScreenOn()) {
    screenWake();
  }
}

void autoScreenTick() {
  // Treat sel==3 as "Off"
  const uint8_t sel = (uint8_t)autoScreenTimeoutSel;

  const uint32_t timeout = autoScreenTimeoutMsForSel(sel);
  if (timeout == 0) return;

  const uint32_t now = millis();

  // If already off, nothing to do (wake is handled by noteUserActivity / shake-to-wake)
  if (!isScreenOn()) return;

  if ((uint32_t)(now - g_lastInputActivityMs) >= timeout) {
    screenSleep();
  }
}
