#pragma once

struct InputState;

void doFactoryResetNow();

void handleFactoryResetInput(InputState& in);
void factoryResetSettingsTick(InputState& in);
void factoryResetResetUiState();
bool factoryResetSystemSettingsHook(InputState& input, int systemSettingsIndex);
