// menu_actions.h
#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "ui_defs.h"
#include "ui_state_utils.h" 

struct InputState;
enum class SettingsPage : uint8_t;

// -----------------------------------------------------------------------------
// Public entry points used outside menu_actions.cpp
// -----------------------------------------------------------------------------

// Main dispatcher (called from loop)
bool handleMenuInput(InputState& in);
void handleSleepScreenInput(InputState &input);
void handleDeathInput(InputState &input);
void handleBurialInput(InputState &in);
void handleChoosePetInput(InputState &in);
void handleNamePetInput(InputState &in);

// Power menu
void openPowerMenuFromHere(uint32_t now);

// Controls help
void openControlsHelpFromSettings();
void openControlsHelpFromAnywhere();

// Boot flow
void bootWizardBegin(UIState afterState, Tab afterTab);
void beginForcedSetTimeBootGate(UIState returnState, Tab returnTab);
extern void settingsCycleTimeZone(int dir);

// Console
void openConsoleWithReturn(UIState returnState, Tab returnTab, bool retToSettings, SettingsPage retSettingsPage);

// Factory reset
void doFactoryResetNow();

// Feed action
void feedUseSelected();

void menuActionsSuppressMenuForMs(uint32_t ms);
