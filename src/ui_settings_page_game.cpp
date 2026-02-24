#include "ui_settings_pages.h"

#include "input.h"
#include "app_state.h"
#include "settings_flow_state.h"
#include "ui_runtime.h"
#include "sound.h"
#include "sdcard.h"
#include "save_manager.h"
#include "led_status.h"
#include "game_options_state.h"

namespace UiSettingsPages {

void Handle_GAME(InputState& input, int move) {
  (void)move;

  const int totalItems = 3;

  int mv = input.encoderDelta;
  if (input.upOnce) mv = -1;
  if (input.downOnce) mv = 1;

  if (mv != 0) {
    g_app.gameOptionsIndex += mv;
    if (g_app.gameOptionsIndex < 0) g_app.gameOptionsIndex = totalItems - 1;
    if (g_app.gameOptionsIndex >= totalItems) g_app.gameOptionsIndex = 0;

    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (input.selectOnce || input.encoderPressOnce) {
    if (g_app.gameOptionsIndex == 0) {
      g_app.decayModeIndex              = (int)saveManagerGetDecayMode();
      g_settingsFlow.settingsReturnPage = SettingsPage::GAME;
      g_settingsFlow.settingsPage       = SettingsPage::DECAY_MODE;
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (g_app.gameOptionsIndex == 1) {
      petDeathEnabled = !petDeathEnabled;
      saveSettingsToSD();
      saveManagerMarkDirty();
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (g_app.gameOptionsIndex == 2) {
      ledAlertsEnabled = !ledAlertsEnabled;
#if LED_STATUS_ENABLED
      ledUpdatePetStatus(LED_PET_OFF);
#endif
      saveSettingsToSD();
      saveManagerMarkDirty();
      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }
  }
}

} // namespace UiSettingsPages