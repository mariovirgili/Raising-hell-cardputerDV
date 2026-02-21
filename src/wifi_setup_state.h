// wifi_setup_state.h
#pragma once

#include <stdint.h>

// Dedicated WiFi UI/Wizard state.
// This replaces the old g_ui.wifi* bucket fields.

struct WifiSetupState {
  // WiFi settings page cursor (Settings -> WiFi)
  int wifiSettingsIndex = 0;

  // Wizard stage: 0 = SSID, 1 = Password
  uint8_t setupStage = 0;

  // Buffers
  char ssid[33] = {0};
  char pass[65] = {0};
  char buf[65]  = {0};
};

extern WifiSetupState g_wifi;
extern bool g_wifiSetupFromBootWizard;

// -----------------------------------------------------------------------------
// Legacy reference aliases
// -----------------------------------------------------------------------------
// Keep these for older code that still uses bare names like wifiSetupBuf.
// IMPORTANT: do NOT use macros for these names.

extern int& wifiSettingsIndex;
extern uint8_t& wifiSetupStage;
extern char (&wifiSetupSsid)[33];
extern char (&wifiSetupPass)[65];
extern char (&wifiSetupBuf)[65];
