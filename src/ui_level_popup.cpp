#include "ui_level_popup.h"
#include "app_state.h"
#include "pet.h"
#include "graphics.h"
#include "ui_invalidate.h"

// Level Up Popup
static uint16_t s_lastShownLevel = 0;
static bool     s_inited = false;

void uiInitLevelPopupTracker() {
  s_lastShownLevel = (uint16_t)pet.level;
  s_inited = true;
}

void uiMaybeShowLevelUpPopup() {
  const uint16_t cur = (uint16_t)pet.level;

  // If init wasn't called yet (boot order, unusual flow), baseline here.
  if (!s_inited) {
    s_lastShownLevel = cur;
    s_inited = true;
    return;
  }

  // Console is a text-capture mode. Never open modal popups there.
  // If the level changes via console commands (setlevel), just update baseline.
  if (g_app.uiState == UIState::CONSOLE) {
    s_lastShownLevel = cur;

    // Safety: if something already activated it, kill it so we can't get stuck.
    if (uiIsLevelUpPopupActive()) {
      uiDismissLevelUpPopup();
    }
    return;
  }

  // If level increased, show popup once (shows the new/current level)
  if (cur > s_lastShownLevel) {
    s_lastShownLevel = cur;
    uiShowLevelUpPopup(cur);
    requestUIRedraw();   // ensure it displays immediately
  }
}
