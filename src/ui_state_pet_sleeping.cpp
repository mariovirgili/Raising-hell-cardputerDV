#include "ui_state_pet_sleeping.h"

#include "app_state.h"
#include "input.h"
#include "save_manager.h"
#include "settings_flow_state.h"
#include "sound.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"
#include "ui_input_common.h"
#include "pet.h"
#include "settings_nav_state.h"

// These exist elsewhere; keep as forward decls for now.
bool powerMenuSleepWakeSuppressedNow();

void uiPetSleepingHandle(InputState& input)
{
  // Suppress immediate wake after returning from power menu
  if (powerMenuSleepWakeSuppressedNow()) {
    uiDrainKb(input);
    input.clearEdges();
    return;
  }

  // Wake detection: use selectHeld rising edge (robust even if isChange() misses)
  static bool s_prevSelectHeld = false;
  const bool enterEdge         = (input.selectHeld && !s_prevSelectHeld);
  s_prevSelectHeld             = input.selectHeld;

  if (enterEdge || input.encoderPressOnce || input.selectOnce) {
    pet.isSleeping        = false;
    g_app.isSleeping      = false;
    g_app.sleepingByTimer = false;

    saveManagerMarkDirty();

    g_app.uiState    = UIState::PET_SCREEN;
    g_app.currentTab = Tab::TAB_PET;
    requestUIRedraw();

    g_app.sleepUntilRested   = false;
    g_app.sleepUntilAwakened = false;
    g_app.sleepTargetEnergy  = 0;

    uiSwallowTypingAndEdges(input);
    return;
  }

  // ESC opens Settings (without waking)
  if (input.escOnce) {
    resetSettingsNav(true);
    g_settingsFlow.settingsPage = SettingsPage::TOP;
    g_app.uiState               = UIState::SETTINGS;
    requestUIRedraw();

    uiDrainKb(input);
    inputForceClear();

    input.escOnce  = false;
    input.menuOnce = false;
    return;
  }

  // Otherwise swallow
  uiDrainKb(input);
  input.clearEdges();
}