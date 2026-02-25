#include "ui_input_router.h"

#include "app_state.h"
#include "input.h"

#include "ui_input_interceptors.h"

// UI state handlers
#include "flow_controls_help.h"
#include "ui_state_burial.h"
#include "ui_state_choose_pet.h"
#include "ui_state_console.h"
#include "ui_state_death.h"
#include "ui_state_inventory.h"
#include "ui_state_mini_game.h"
#include "ui_state_name_pet.h"
#include "ui_state_pet.h"
#include "ui_state_pet_sleeping.h"
#include "ui_state_settings.h"
#include "ui_state_shop.h"
#include "ui_state_sleep_menu.h"
#include "ui_state_tab_driven.h"
#include "flow_boot_wifi.h"

// Flows / editors
#include "flow_time_editor.h"

// Local “safe swallow” for boot/flow states that should not accept random edges
static inline void drainKb(InputState &in)
{
  while (in.kbHasEvent())
    (void)in.kbPop();
}

static inline void swallowEdgesOnly(InputState &in)
{
  drainKb(in);
  in.clearEdges();
}

static inline bool isNoInputState(UIState s)
{
  switch (s)
  {
  case UIState::BOOT:
  case UIState::POWER_MENU: 
  case UIState::WIFI_SETUP:
  case UIState::HATCHING:
  case UIState::EVOLUTION:
    return true;
  default:
    return false;
  }
}

bool uiHandleInput(InputState &in)
{
  // Phase 1: global interceptors (power menu, ESC->settings, sleep gate, etc)
  if (uiHandleGlobalInterceptors(in))
    return true;

  // Explicitly “no input” states: swallow edges so nothing leaks in from buffers.
  if (isNoInputState(g_app.uiState))
  {
    swallowEdgesOnly(in);
    return true;
  }

  // Phase 2: UI state dispatch
  switch (g_app.uiState)
  {
  case UIState::HOME:
    uiTabDrivenHandle(in);
    return true;

  case UIState::PET_SCREEN:
    uiPetScreenHandle(in);
    return true;

  case UIState::PET_SLEEPING:
    uiPetSleepingHandle(in);
    return true;

  case UIState::SLEEP_MENU:
    uiSleepMenuHandle(in);
    return true;

  case UIState::INVENTORY:
    uiInventoryHandle(in);
    return true;

  case UIState::SHOP:
    uiShopHandle(in);
    return true;

  case UIState::SETTINGS:
    uiSettingsHandle(in);
    return true;

  case UIState::CONSOLE:
    uiConsoleHandle(in);
    return true;

  case UIState::DEATH:
    uiDeathHandle(in);
    return true;

  case UIState::BURIAL_SCREEN:
    uiBurialHandle(in);
    return true;

  case UIState::MINI_GAME:
    uiMiniGameHandle(in);
    return true;

  case UIState::CHOOSE_PET:
    uiChoosePetHandle(in);
    return true;

  case UIState::NAME_PET:
    uiNamePetHandle(in);
    return true;

  case UIState::SET_TIME:
    uiSetTimeHandle(in);
    return true;

  case UIState::CONTROLS_HELP:
    uiControlsHelpHandle(in);
    return true;

    case UIState::BOOT_WIFI_PROMPT:
  uiBootWifiPromptHandle(in);
  return true;

case UIState::BOOT_WIFI_WAIT:
  uiBootWifiWaitHandle(in);
  return true;

case UIState::BOOT_TZ_PICK:
  uiBootTzPickHandle(in);
  return true;

case UIState::BOOT_NTP_WAIT:
  uiBootNtpWaitHandle(in);
  return true;
  default:
    break;
  }

  return false;
}