// flow_power_menu.cpp
#include "flow_power_menu.h"

#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "app_state.h"
#include "auto_screen.h"
#include "input.h"
#include "input_activity_state.h"
#include "save_manager.h"
#include "settings_flow_state.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"
#include "wifi_power.h"
#include "sound.h"
#include "display.h"
#include "graphics.h" 
#include "pet.h"
#include "sleep_state.h"

// Suppress wake edges briefly when returning to PET_SLEEPING from overlays
static uint32_t s_suppressSleepWakeUntilMs = 0;

bool powerMenuSleepWakeSuppressedNow() {
  return (int32_t)(millis() - s_suppressSleepWakeUntilMs) < 0;
}

void openPowerMenuFromHere(uint32_t nowMs) {
  // Snapshot return target BEFORE switching states.
  const bool sleepingNow = (pet.isSleeping || isSleeping || g_app.uiState == UIState::PET_SLEEPING);

  g_settingsFlow.powerMenuReturnToSleep = sleepingNow;
  g_settingsFlow.powerMenuReturnState   = g_app.uiState;
  g_settingsFlow.powerMenuReturnTab     = g_app.currentTab;

  // If screen is off, wake it first so the menu is visible.
  if (!isScreenOn()) {
    SET_SCREEN_POWER(true);
    setLastInputActivityMs(nowMs);
  }

  powerMenuIndex       = 0;
  g_app.uiState        = UIState::POWER_MENU;
  g_app.uiNeedsRedraw  = true;

  requestFullUIRedraw();

  // Prevent the long-press from also producing stray edges
  inputForceClear();
  clearInputLatch();
}

static inline void drainKb(InputState& in) {
  while (in.kbHasEvent()) (void)in.kbPop();
}

void handlePowerMenuInput(InputState& input) {
  // Exit keys:
  //  - ESC / MENU edges
  //  - Q and DEL/BACKSPACE from kb queue
  bool exitPressed = (input.menuOnce || input.escOnce);

  if (!exitPressed) {
    while (input.kbHasEvent()) {
      KeyEvent e = input.kbPop();
      const uint8_t c = e.code;

      const bool isQ = (c == 'q' || c == 'Q');
      const bool isDelOrBackspace =
        (c == (uint8_t)KEY_BACKSPACE) || (c == '\b') || (c == 127) || (c == 0x2A);
      const bool isEsc = (c == 27); // sometimes comes through as ASCII 27

      if (isQ || isDelOrBackspace || isEsc) {
        exitPressed = true;
        break;
      }
    }
  } else {
    drainKb(input);
  }

  if (exitPressed) {
    const bool returningToSleep = g_settingsFlow.powerMenuReturnToSleep;

    if (returningToSleep) {
      g_app.uiState    = UIState::PET_SLEEPING;
      g_app.currentTab = Tab::TAB_PET;
      s_suppressSleepWakeUntilMs = millis() + 400;
    } else {
      g_app.uiState    = g_settingsFlow.powerMenuReturnState;
      g_app.currentTab = g_settingsFlow.powerMenuReturnTab;
    }

    g_settingsFlow.powerMenuReturnToSleep = false;

    requestFullUIRedraw();

    input.clearEdges();
    inputForceClear();
    clearInputLatch();
    return;
  }

  // Navigation: 2 items: [0]=Reboot, [1]=Shut Down
  const int itemCount = 2;

  int mv = 0;
  if (input.upOnce) mv = -1;
  if (input.downOnce) mv = +1;
  if (input.encoderDelta < 0) mv = -1;
  if (input.encoderDelta > 0) mv = +1;

  if (mv != 0) {
    powerMenuIndex += mv;
    while (powerMenuIndex < 0) powerMenuIndex += itemCount;
    powerMenuIndex %= itemCount;

    requestUIRedraw();
    requestUIRedraw();
    playBeep();

    input.clearEdges();
    clearInputLatch();
    return;
  }

  const bool doSelect = (input.selectOnce || input.encoderPressOnce);
  if (doSelect) {
    playBeep();

    input.clearEdges();
    inputForceClear();
    clearInputLatch();

    if (powerMenuIndex == 0) {
      saveManagerForce();
      delay(40);
      ESP.restart();
      return;
    }

    if (powerMenuIndex == 1) {
      saveManagerForce();
      applyWifiPower(false);

      SET_SCREEN_POWER(false);

      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // active-low

      rtc_gpio_deinit(GPIO_NUM_0);
      rtc_gpio_pullup_en(GPIO_NUM_0);
      rtc_gpio_pulldown_dis(GPIO_NUM_0);

      delay(60);
      esp_deep_sleep_start();
      return;
    }
    return;
  }

  // Otherwise swallow typing so random keys don't affect anything
  drainKb(input);
  input.clearEdges();
  clearInputLatch();
}
