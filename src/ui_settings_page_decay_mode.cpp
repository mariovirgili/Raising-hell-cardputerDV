#include "ui_settings_pages.h"

#include "input.h"
#include "app_state.h"
#include "settings_flow_state.h"
#include "ui_runtime.h"
#include "sound.h"
#include "save_manager.h"

namespace UiSettingsPages {

void Handle_DECAY_MODE(InputState& input, int move) {
  (void)move;

  const int kCount = 6;

  int mv = input.encoderDelta;
  if (input.upOnce) mv = -1;
  if (input.downOnce) mv = 1;

  if (mv != 0) {
    g_app.decayModeIndex =
      (g_app.decayModeIndex + (mv < 0 ? (kCount - 1) : 1)) % kCount;
    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (input.selectOnce || input.encoderPressOnce) {
    saveManagerSetDecayMode((uint8_t)g_app.decayModeIndex);
    g_settingsFlow.settingsPage = g_settingsFlow.settingsReturnPage;
    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (input.menuOnce || input.escOnce) {
    g_settingsFlow.settingsPage = g_settingsFlow.settingsReturnPage;
    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }
}

} // namespace UiSettingsPages