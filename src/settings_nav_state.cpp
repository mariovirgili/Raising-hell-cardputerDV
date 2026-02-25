#include "settings_nav_state.h"

#include "app_state.h"
#include "flow_factory_reset.h"
#include "settings_flow_state.h"
#include "wifi_setup_state.h" // for g_wifi (if that’s where it lives)

void resetSettingsNav(bool resetTopIndex) {
  g_settingsFlow.settingsPage     = SettingsPage::TOP;
  g_app.screenSettingsIndex       = 0;
  g_app.systemSettingsIndex       = 0;
  g_wifi.wifiSettingsIndex        = 0;
  g_app.gameOptionsIndex          = 0;
  g_app.autoScreenIndex           = 0;
  g_app.decayModeIndex            = 0;

  factoryResetResetUiState();

  if (resetTopIndex) g_app.settingsIndex = 0;
}