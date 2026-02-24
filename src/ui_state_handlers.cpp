#include "ui_state_handlers.h"
#include "flow_boot_wifi.h"
#include "flow_controls_help.h"
#include "ui_state_burial.h"
#include "ui_state_choose_pet.h"
#include "ui_state_console.h"
#include "ui_state_death.h"
#include "ui_state_mini_game.h"
#include "ui_state_name_pet.h"
#include "ui_state_pet.h"
#include "ui_state_pet_sleeping.h"
#include "ui_state_settings.h"
#include "ui_state_sleep_menu.h"
#include "ui_state_tab_driven.h"
#include "ui_state_wifi_setup.h"
#include "flow_time_editor.h"
#include "debug.h"

bool uiDispatchToStateHandler(UIState st, InputState &in)
{
  switch (st)
  {
  case UIState::SETTINGS:
    uiSettingsHandle(in);
    return true;

  case UIState::CONSOLE:
    uiConsoleHandle(in);
    return true;

  case UIState::WIFI_SETUP:
    uiWifiSetupHandle(in);
    return true;

  case UIState::PET_SCREEN:
    uiTabDrivenHandle(in);
    return true;

  case UIState::MINI_GAME:
    uiMiniGameHandle(in);
    return true;

  case UIState::SLEEP_MENU:
    uiSleepMenuHandle(in);
    return true;

  case UIState::PET_SLEEPING:
    uiPetSleepingHandle(in);
    return true;

  case UIState::HOME:
    uiTabDrivenHandle(in);
    return true;

  case UIState::DEATH:
    uiDeathHandle(in);
    return true;

  case UIState::BURIAL_SCREEN:
    uiBurialHandle(in);
    return true;

  case UIState::CHOOSE_PET:
    uiChoosePetHandle(in);
    return true;

  case UIState::NAME_PET:
    uiNamePetHandle(in);
    return true;

  case UIState::CONTROLS_HELP:
    handleControlsHelpInput(in);
    return true;

  case UIState::BOOT_WIFI_PROMPT:
    handleBootWifiPromptInput(in);
    return true;

  case UIState::BOOT_WIFI_WAIT:
    handleBootWifiWaitInput(in);
    return true;

  case UIState::BOOT_TZ_PICK:
    handleBootTzPickInput(in);
    return true;

  case UIState::BOOT_NTP_WAIT:
    handleBootNtpWaitInput(in);
    return true;

    case UIState::SET_TIME:
    handleTimeSetInput(in);
    return true;
    
  case UIState::INVENTORY:
    uiTabDrivenHandle(in);
    return true;

  case UIState::SHOP:
    uiTabDrivenHandle(in);
    return true;

    default:
    #if !PUBLIC_BUILD
        DBGLN_ON("Unhandled UIState in dispatcher");
    #endif
        return false;
  }
}