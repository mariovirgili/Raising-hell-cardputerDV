#include "ui_state_wifi_setup.h"

#include "input.h"

#include "app_state.h"
#include "settings_flow_state.h"
#include "ui_defs.h"

#include "sound.h"
#include "ui_runtime.h"
#include "ui_input_utils.h"

#include "save_manager.h"
#include "wifi_power.h"
#include "wifi_store.h"
#include "wifi_time.h"
#include "wifi_setup_state.h"

#include <cstring>
#include <cstdint>

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

static void wifiSetupAppendChar(char c) {
  const int curLen = (int)strlen(g_wifi.buf);
  const int maxLen = (int)sizeof(g_wifi.buf) - 1;
  if (curLen >= maxLen) return;

  g_wifi.buf[curLen]     = c;
  g_wifi.buf[curLen + 1] = '\0';
}

static void wifiSetupBackspace() {
  const int curLen = (int)strlen(g_wifi.buf);
  if (curLen <= 0) return;
  g_wifi.buf[curLen - 1] = '\0';
}

// ----------------------------------------------------------------------------
// WiFi Setup (SSID/PASS entry) handler
// ----------------------------------------------------------------------------

void uiWifiSetupHandle(InputState &in) {
  // ESC cancels and returns to the caller (boot wizard or settings)
  if (in.escOnce || in.menuOnce) {
    if (g_wifiSetupFromBootWizard) {
      g_wifiSetupFromBootWizard = false;
      g_app.uiState             = UIState::BOOT_WIFI_PROMPT;
    } else {
      g_app.uiState = UIState::SETTINGS;
    }

    requestUIRedraw();
    inputSetTextCapture(false);
    g_textCaptureMode = false;

    uiDrainKb(in);
    clearInputLatch();
    playBeep();
    return;
  }

  bool changed      = false;
  bool pressedEnter = false;

  // Pull key events from the keyboard queue
  while (in.kbHasEvent()) {
    KeyEvent ev = in.kbPop();
    const uint8_t kc = ev.code;

    if (kc == 0) break;

    // Enter
    if (kc == RH_KEY_ENTER || kc == '\n' || kc == '\r') {
      pressedEnter = true;
      continue;
    }

    // Backspace
    if (kc == RH_KEY_BACKSPACE || kc == 0x08) {
      const int before = (int)strlen(g_wifi.buf);
      wifiSetupBackspace();
      if ((int)strlen(g_wifi.buf) != before) changed = true;
      continue;
    }

    // Printable ASCII
    if (kc >= 32 && kc <= 126) {
      const int before = (int)strlen(g_wifi.buf);
      wifiSetupAppendChar((char)kc);
      if ((int)strlen(g_wifi.buf) != before) changed = true;
      continue;
    }

    // ignore everything else
  }

  if (changed) {
    requestUIRedraw();
  }

  // Enter (or select/press) advances stages
  const bool advance = pressedEnter || in.selectOnce || in.encoderPressOnce;
  if (!advance) return;

  if (g_wifi.setupStage == 0) {
    // Save SSID
    strncpy(g_wifi.ssid, g_wifi.buf, sizeof(g_wifi.ssid) - 1);
    g_wifi.ssid[sizeof(g_wifi.ssid) - 1] = '\0';

    g_wifi.buf[0]     = '\0';
    g_wifi.setupStage = 1;

    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (g_wifi.setupStage == 1) {
    // Save PASS
    strncpy(g_wifi.pass, g_wifi.buf, sizeof(g_wifi.pass) - 1);
    g_wifi.pass[sizeof(g_wifi.pass) - 1] = '\0';

    // Persist + enable wifi
    wifiStoreSave(g_wifi.ssid, g_wifi.pass);

    wifiSetEnabled(true);
    settingsSetWifiEnabled(true);
    applyWifiPower(true);

    // Return out
    if (g_wifiSetupFromBootWizard) {
      g_wifiSetupFromBootWizard = false;
      g_app.uiState             = UIState::BOOT_WIFI_WAIT;
    } else {
      g_app.uiState = UIState::SETTINGS;
    }

    requestUIRedraw();
    inputSetTextCapture(false);
    g_textCaptureMode = false;

    uiDrainKb(in);
    clearInputLatch();
    playBeep();
    return;
  }
}