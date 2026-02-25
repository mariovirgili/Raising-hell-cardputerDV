#pragma once

#include <stdint.h>

struct InputState;
enum class UIState : uint8_t;
enum class Tab : uint8_t;
enum class SettingsPage : uint8_t;

// Open the time editor from Settings, returning to Settings on exit.
void beginSetTimeEditorFromSettings(SettingsPage returnSettingsPage, UIState returnState, Tab returnTab);

// Boot gate: force no-cancel editor and return to a UI state/tab (not Settings).
void beginForcedSetTimeBootGate(UIState returnState, Tab returnTab);

// Existing handler for UIState::SET_TIME
void handleTimeSetInput(InputState& in);

void uiSetTimeHandle(InputState& in);
