#pragma once
#include <stdint.h>
#include <stdbool.h>

// 0=5m, 1=30m, 2=1h, 3=Off
extern uint8_t  autoScreenTimeoutSel;

// Legacy/diagnostic flag (keep for compatibility; your real pipeline uses isScreenOn()).
extern bool     g_screenIsOff;

// UI-friendly string for current selection.
const char* autoScreenToText(uint8_t sel);

// Helpers
uint32_t autoScreenTimeoutMsForSel(uint8_t sel);
uint32_t autoScreenTimeoutMs();

// Called whenever user input occurs (updates timestamp + wakes screen if needed)
void noteUserActivity();

// Called from loop to blank screen after inactivity
void autoScreenTick();

// Exported screen power helpers used across the UI
void screenWake();
void screenSleep();

// Enable/disable auto screen-off behavior (and query current setting)
bool autoScreenIsEnabled();
void autoScreenSetEnabled(bool enabled);
