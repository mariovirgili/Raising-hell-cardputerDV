#include "ui_state_handlers.h"

#include "debug.h"
#include "flow_boot_wifi.h"
#include "flow_controls_help.h"
#include "flow_power_menu.h"
#include "flow_time_editor.h"

#include "ui_state_burial.h"
#include "ui_state_choose_pet.h"
#include "ui_state_console.h"
#include "ui_state_death.h"
#include "ui_state_mini_game.h"
#include "ui_state_name_pet.h"
#include "ui_state_pet_sleeping.h"
#include "ui_state_settings.h"
#include "ui_state_sleep_menu.h"
#include "ui_state_tab_driven.h"
#include "ui_state_wifi_setup.h"

// Handler table indexed by UIState numeric value (see ui_defs.h).
// IMPORTANT: Keep this in exact numeric order to match UIState values.
// Any nullptr entry preserves current behavior: dispatcher returns false.
static const StateHandlerFn kHandlers[] = {
  /*  0: UIState::BOOT             */ nullptr,
  /*  1: UIState::HOME             */ uiTabDrivenHandle,
  /*  2: UIState::PET_SCREEN       */ uiTabDrivenHandle,
  /*  3: UIState::POWER_MENU       */ handlePowerMenuInput,
  /*  4: UIState::MINI_GAME        */ uiMiniGameHandle,
  /*  5: UIState::CHOOSE_PET       */ uiChoosePetHandle,
  /*  6: UIState::NAME_PET         */ uiNamePetHandle,
  /*  7: UIState::WIFI_SETUP       */ uiWifiSetupHandle,
  /*  8: UIState::DEATH            */ uiDeathHandle,
  /*  9: UIState::BURIAL_SCREEN    */ uiBurialHandle,
  /* 10: UIState::PET_SLEEPING     */ uiPetSleepingHandle,
  /* 11: UIState::SETTINGS         */ uiSettingsHandle,
  /* 12: UIState::CONSOLE          */ uiConsoleHandle,
  /* 13: UIState::INVENTORY        */ uiTabDrivenHandle,
  /* 14: UIState::SHOP             */ uiTabDrivenHandle,
  /* 15: UIState::SLEEP_MENU       */ uiSleepMenuHandle,
  /* 16: UIState::SET_TIME         */ handleTimeSetInput,
  /* 17: UIState::HATCHING         */ nullptr,
  /* 18: UIState::CONTROLS_HELP    */ handleControlsHelpInput,
  /* 19: UIState::BOOT_WIFI_PROMPT */ handleBootWifiPromptInput,
  /* 20: UIState::BOOT_WIFI_WAIT   */ handleBootWifiWaitInput,
  /* 21: UIState::BOOT_TZ_PICK     */ handleBootTzPickInput,
  /* 22: UIState::BOOT_NTP_WAIT    */ handleBootNtpWaitInput,
  /* 23: UIState::EVOLUTION        */ nullptr,
};

// Keep the handler table in sync with UIState enum without requiring a COUNT sentinel.
// IMPORTANT: Update kExpectedStates whenever UIState grows/shrinks.
static constexpr int kExpectedStates = (int)(sizeof(kHandlers) / sizeof(kHandlers[0]));
static_assert(kExpectedStates > 0, "kHandlers must not be empty");          
bool uiDispatchToStateHandler(UIState state, InputState& in)
{
  const int idx = (int)state;
  const int count = (int)(sizeof(kHandlers) / sizeof(kHandlers[0]));

  if (idx < 0 || idx >= count) {
#if !PUBLIC_BUILD
    DBGLN_ON("UIState out of range in dispatcher");
#endif
    return false;
  }

  const StateHandlerFn fn = kHandlers[idx];
  if (!fn) {
#if !PUBLIC_BUILD
    DBGLN_ON("Unhandled UIState in dispatcher");
#endif
    return false;
  }

  fn(in);
  return true;
}