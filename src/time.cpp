#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include "wifi_time.h"

#include "time_state.h"
#include "timezone.h"

static bool s_timeInitDone = false;

// -------------------------------------------------------------
// Initialize NTP (legacy entry point)
// NOTE: You now primarily use wifi_time.cpp (wifiTimeInit/wifiTimeTick).
// This function is kept for compatibility if anything still calls it.
// -------------------------------------------------------------
void initTimeNTP() {
  static bool started = false;
  if (started) return;

if (!wifiIsConnectedNow()) return;

  // Apply timezone BEFORE starting SNTP
  applyTimezoneIndex(tzIndex);

  // Standard SNTP sources
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  started = true;
  s_timeInitDone = true;
}

// -------------------------------------------------------------
// Update internal HUD clock (called from main loop)
// -------------------------------------------------------------
void updateTime() {
  time_t now = time(nullptr);
  if (now <= 0) return;

  tm tmNow;
  localtime_r(&now, &tmNow);

  currentHour = tmNow.tm_hour;
  currentMinute = tmNow.tm_min;
  lastMinuteUpdate = millis();
}

// -------------------------------------------------------------
// Legacy helpers — no longer used (system time + NVS anchor is canonical)
// Kept as no-ops to avoid accidental EEPROM-based regressions.
// -------------------------------------------------------------
void setTime(int hour, int minute) {
  (void)hour;
  (void)minute;
  // NO-OP: Set Time uses settimeofday() + saveTimeAnchor().
}

void loadTimeFromEEPROM() {
  // NO-OP: time anchor restore replaces EEPROM hour/min.
}
