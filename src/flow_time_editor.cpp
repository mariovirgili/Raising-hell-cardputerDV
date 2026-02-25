#include "flow_time_editor.h"

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>
#include <cstring>

#include "app_state.h"
#include "input.h"
#include "save_manager.h"
#include "settings_flow_state.h"
#include "time_editor_state.h"
#include "time_state.h"
#include "time_persist.h"
#include "ui_runtime.h"
#include "graphics.h"
#include "new_pet_flow_state.h"

static bool         g_setTimeReturnToSettings   = true;
static SettingsPage g_setTimeReturnSettingsPage = SettingsPage::SYSTEM;

// ------------------------------------------------------------------
// Set Time editor entry/return bookkeeping
// ------------------------------------------------------------------
static void beginSetTimeEditor(bool forceNoCancel,
                               bool returnToSettings,
                               SettingsPage returnSettingsPage,
                               UIState returnState,
                               Tab returnTab) {
  // Ensure arrow keys / encoder map to UI navigation (not keyboard text capture)
  inputSetTextCapture(false);
  g_textCaptureMode = false;

  // Initialize from current local time if valid, else a sane default
  time_t now = time(nullptr);
  if (now > 1700000000) { // ~late 2023, "valid enough"
    localtime_r(&now, &g_setTimeTm);
  } else {
    memset(&g_setTimeTm, 0, sizeof(g_setTimeTm));
    g_setTimeTm.tm_year = 2026 - 1900;
    g_setTimeTm.tm_mon  = 0; // Jan
    g_setTimeTm.tm_mday = 1;
    g_setTimeTm.tm_hour = 12;
    g_setTimeTm.tm_min  = 0;
    g_setTimeTm.tm_sec  = 0;
  }

  g_setTimeField  = 0; // 0..4 fields, 5 = OK
  g_setTimeActive = true;

  g_setTimeForceNoCancel = forceNoCancel;

  g_setTimeReturnValid = true;
  g_setTimeReturnState = (uint8_t)returnState;
  g_setTimeReturnTab   = (uint8_t)returnTab;

  g_setTimeReturnToSettings   = returnToSettings;
  g_setTimeReturnSettingsPage = returnSettingsPage;

  // Discard residue so it won't interfere
  clearInputLatch();
  inputForceClear();

  g_app.uiState = UIState::SET_TIME;
  requestFullUIRedraw();
  clearInputLatch();
}

void beginSetTimeEditorFromSettings(SettingsPage returnSettingsPage, UIState returnState, Tab returnTab) {
  beginSetTimeEditor(
    false,                 // forceNoCancel
    true,                  // returnToSettings
    returnSettingsPage,
    returnState,
    returnTab
  );
  clearInputLatch();
}

void beginForcedSetTimeBootGate(UIState returnState, Tab returnTab) {
  inputSetTextCapture(false);
  g_textCaptureMode = false;

  beginSetTimeEditor(
    true,                 // forceNoCancel
    false,                // returnToSettings
    SettingsPage::SYSTEM, // unused when returnToSettings=false
    returnState,
    returnTab
  );

  clearInputLatch();
}

// ==================================================================
// TIME SETTING
// ==================================================================
int daysInMonth(int year, int mon0) {
  static const int d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  int dm = d[mon0];
  if (mon0 == 1) {
    bool leap = ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
    if (leap) dm = 29;
  }
  return dm;
}

static void adjustField(int delta) {
  int year = g_setTimeTm.tm_year + 1900;
  int mon0 = g_setTimeTm.tm_mon;  // 0..11
  int day  = g_setTimeTm.tm_mday; // 1..31

  auto wrapInt = [&](int v, int lo, int hi) -> int {
    const int range = (hi - lo + 1);
    while (v < lo) v += range;
    while (v > hi) v -= range;
    return v;
  };

  switch (g_setTimeField) {
    case 0: {
      year = wrapInt(year + delta, 2020, 2099);
      g_setTimeTm.tm_year = year - 1900;

      const int dim = daysInMonth(year, mon0);
      g_setTimeTm.tm_mday = wrapInt(g_setTimeTm.tm_mday, 1, dim);
      break;
    }

    case 1: {
      mon0 = wrapInt(mon0 + delta, 0, 11);
      g_setTimeTm.tm_mon = mon0;

      const int dim = daysInMonth(year, mon0);
      g_setTimeTm.tm_mday = wrapInt(g_setTimeTm.tm_mday, 1, dim);
      break;
    }

    case 2: {
      const int dim = daysInMonth(year, mon0);
      day = wrapInt(day + delta, 1, dim);
      g_setTimeTm.tm_mday = day;
      break;
    }

    case 3:
      g_setTimeTm.tm_hour = wrapInt(g_setTimeTm.tm_hour + delta, 0, 23);
      break;

    case 4:
      g_setTimeTm.tm_min = wrapInt(g_setTimeTm.tm_min + delta, 0, 59);
      break;

    default: break;
  }
}

void handleTimeSetInput(InputState &in) {
  // Map keyboard arrow/enter keys into edge-style behavior
  bool kbLeft = false;
  bool kbRight = false;
  bool kbUp = false;
  bool kbDown = false;
  bool kbEnter = false;

  while (in.kbHasEvent()) {
    const KeyEvent ev = in.kbPop();
    const uint8_t c = ev.code;

    if (c == '\n' || c == '\r') kbEnter = true;
    else if (c == 0x1B) {} // ignore ESC here (handled elsewhere)
    else if (c == 0x08) {} // ignore backspace
    else if (c == 'A') kbUp = true;     // adjust if your arrow mapping differs
    else if (c == 'B') kbDown = true;
    else if (c == 'C') kbRight = true;
    else if (c == 'D') kbLeft = true;
  }

  if (in.escOnce || in.menuOnce) {
    if (g_setTimeForceNoCancel) {
      requestUIRedraw();
      clearInputLatch();
      return;
    }

    if (g_setTimeReturnValid) {
      if (g_setTimeReturnToSettings) {
        g_app.uiState               = UIState::SETTINGS;
        g_settingsFlow.settingsPage = g_setTimeReturnSettingsPage;
      } else {
        g_app.uiState    = (UIState)g_setTimeReturnState;
        g_app.currentTab = (Tab)g_setTimeReturnTab;
      }
    } else {
      g_app.uiState               = UIState::SETTINGS;
      g_settingsFlow.settingsPage = SettingsPage::SYSTEM;
    }

    // If returning to egg select, require clean release before hatch
    if (g_app.uiState == UIState::CHOOSE_PET) {
      g_choosePetBlockHatchUntilRelease = true;
    }

    requestUIRedraw();
    clearInputLatch();
    return;
  }

  if (in.leftOnce || kbLeft) {
    g_setTimeField--;
    if (g_setTimeField < 0) g_setTimeField = 5;
    requestUIRedraw();
    clearInputLatch();
    return;
  }
  if (in.rightOnce  || kbRight) {
    g_setTimeField++;
    if (g_setTimeField > 5) g_setTimeField = 0;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  if (g_setTimeField <= 4) {
    if (in.upOnce || kbUp)   { adjustField(+1); requestUIRedraw(); clearInputLatch(); return; }
    if (in.downOnce || kbDown) { adjustField(-1); requestUIRedraw(); clearInputLatch(); return; }
  }

  if (in.selectOnce || kbEnter ||in.encoderPressOnce) {
    if (g_setTimeField < 5) {
      g_setTimeField++;
      if (g_setTimeField > 5) g_setTimeField = 5;
      requestUIRedraw();
      clearInputLatch();
      return;
    }

    tm tmp = g_setTimeTm;
    tmp.tm_isdst = -1;
    time_t t = mktime(&tmp);

    if (t > 0) {
      timeval tv;
      tv.tv_sec  = t;
      tv.tv_usec = 0;
      settimeofday(&tv, nullptr);

      updateTime();
      saveTimeAnchor();
    }

    g_setTimeActive        = false;
    g_setTimeForceNoCancel = false;

    if (g_setTimeReturnValid) {
      if (g_setTimeReturnToSettings) {
        g_app.uiState               = UIState::SETTINGS;
        g_settingsFlow.settingsPage = g_setTimeReturnSettingsPage;
      } else {
        g_app.uiState    = (UIState)g_setTimeReturnState;
        g_app.currentTab = (Tab)g_setTimeReturnTab;
      }
    } else {
      g_app.uiState               = UIState::SETTINGS;
      g_settingsFlow.settingsPage = SettingsPage::SYSTEM;
    }

    if (g_app.uiState == UIState::CHOOSE_PET) {
      g_choosePetBlockHatchUntilRelease = true;
    }

    requestFullUIRedraw();
    clearInputLatch();
    return;
  }
}

void uiSetTimeHandle(InputState& in) {
  handleTimeSetInput(in);
}