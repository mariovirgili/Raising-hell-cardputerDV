// ui_state_handlers.cpp
#include "ui_state_handlers.h"

#include "input.h"

// These handlers are implemented in menu_actions.cpp (for now).
// These handlers are implemented in menu_actions.cpp (for now).
void handlePetScreen(const InputState& input);
void handleInventoryInput(const InputState& input);
void handleShopInput(const InputState& input);
void handleSleepMenuInput(const InputState& input);
void handleSettingsInput(InputState& input);
void handleMiniGameInput(const InputState& input);

void handleSleepScreenInput(InputState& input);
void handleTimeSetInput(InputState& in);
void handlePowerMenuInput(InputState& input);

void handleConsoleInput(InputState& in);
void handleWifiSetupInput(InputState& in);

void handleDeathInput(InputState& input);
void handleBurialInput(InputState& in);

void handleChoosePetInput(InputState& in);
void handleNamePetInput(InputState& in);

void handleControlsHelpInput(InputState& in);
void handleBootWifiPromptInput(InputState& in);
void handleBootWifiWaitInput(InputState& in);
void handleBootTzPickInput(InputState& in);
void handleBootNtpWaitInput(InputState& in);

bool uiDispatchToStateHandler(UIState state, InputState& in) {
  switch (state) {
    case UIState::PET_SCREEN:        handlePetScreen(in);           return true;
    case UIState::PET_SLEEPING:      handleSleepScreenInput(in);    return true;
    case UIState::INVENTORY:         handleInventoryInput(in);      return true;
    case UIState::SHOP:              handleShopInput(in);           return true;
    case UIState::SLEEP_MENU:        handleSleepMenuInput(in);      return true;
    case UIState::SETTINGS:          handleSettingsInput(in);       return true;
    case UIState::MINI_GAME:         handleMiniGameInput(in);       return true;
    case UIState::SET_TIME:          handleTimeSetInput(in);        return true;
    case UIState::POWER_MENU:        handlePowerMenuInput(in);      return true;
    case UIState::CONSOLE:           handleConsoleInput(in);        return true;
    case UIState::WIFI_SETUP:        handleWifiSetupInput(in);      return true;
    case UIState::DEATH:             handleDeathInput(in);          return true;
    case UIState::BURIAL_SCREEN:     handleBurialInput(in);         return true;
    case UIState::CHOOSE_PET:        handleChoosePetInput(in);      return true;
    case UIState::NAME_PET:          handleNamePetInput(in);        return true;
    case UIState::CONTROLS_HELP:     handleControlsHelpInput(in);   return true;
    case UIState::BOOT_WIFI_PROMPT:  handleBootWifiPromptInput(in); return true;
    case UIState::BOOT_WIFI_WAIT:    handleBootWifiWaitInput(in);   return true;
    case UIState::BOOT_TZ_PICK:      handleBootTzPickInput(in);     return true;
    case UIState::BOOT_NTP_WAIT:     handleBootNtpWaitInput(in);    return true;
    default: break;
  }
  return false;
}