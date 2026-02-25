#include "ui_state_wifi_setup.h"

#include "input.h"

#include "app_state.h"
#include "settings_flow_state.h"
#include "ui_defs.h"

#include "sound.h"
#include "ui_runtime.h"
#include "ui_input_utils.h"
#include "ui_input_common.h"

#include "save_manager.h"
#include "wifi_power.h"
#include "wifi_store.h"
#include "wifi_time.h"
#include "wifi_setup_state.h"

#include <cstring>
#include "ui_input_common.h"

void ui_showMessage(const char* msg);

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

static bool wifiSetupBackspace() {
  const int curLen = (int)strlen(g_wifi.buf);
  if (curLen <= 0) return false;
  g_wifi.buf[curLen - 1] = '\0';
  return true;
}

static void wifiSetupCancel(InputState &in) {
  if (g_wifiSetupFromBootWizard) {
    g_wifiSetupFromBootWizard = false;
    g_app.uiState             = UIState::BOOT_WIFI_PROMPT;
  } else {
    g_app.uiState = UIState::SETTINGS;
  }

  ui_showMessage("Canceled");
  requestUIRedraw();

  inputSetTextCapture(false);
  g_textCaptureMode = false;

  uiDrainKb(in);
  clearInputLatch();
  playBeep();
}

static void wifiSetupAdvanceOrFinish(InputState &in) {
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

// ----------------------------------------------------------------------------
// WiFi Setup (SSID/PASS entry) handler
// ----------------------------------------------------------------------------

void uiWifiSetupHandle(InputState &in) {
  bool changed      = false;
  bool pressedEnter = false;
  bool pressedEsc   = false;

  // Read keyboard events directly. In text-capture mode, the *_Once shortcuts
  // (menuOnce/selectOnce/escOnce) may be suppressed by input_cardputer.cpp,
  // so we must handle Enter/Backspace/Esc via the KeyEvent queue.
  while (in.kbHasEvent()) {
    const KeyEvent ev = in.kbPop();
    const uint8_t  kc = ev.code;

    if (kc == 0) break;

    // Treat ASCII ESC as cancel
    if (kc == 27) {
      pressedEsc = true;
      continue;
    }

    // In your mapping, '`' / '~' normally acts like ESC when NOT in text capture.
    // When text capture is enabled, it becomes a normal character, so we make it cancel here.
    if (kc == '`' || kc == '~') {
      pressedEsc = true;
      continue;
    }

    // Enter variants
    if (kc == '\n' || kc == '\r') {
      pressedEnter = true;
      continue;
    }

    // Backspace variants
    if (kc == '\b' || kc == 127) {
      if (wifiSetupBackspace()) changed = true;
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

  // Also allow hardware/menu flags if they still come through.
  if (in.escOnce || in.menuOnce) pressedEsc = true;

  if (pressedEsc) {
    wifiSetupCancel(in);
    return;
  }

  if (changed) {
    requestUIRedraw();
  }

  // Advance on Enter, or encoder press/select if available.
  const bool advance = pressedEnter || in.selectOnce || in.encoderPressOnce;
  if (!advance) return;

  wifiSetupAdvanceOrFinish(in);
}