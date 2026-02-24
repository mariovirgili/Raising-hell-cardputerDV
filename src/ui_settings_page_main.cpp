#include "ui_settings_pages.h"

#include "app_state.h"
#include "settings_flow_state.h"
#include "ui_defs.h"
#include "ui_input_utils.h"

#include "menu_actions.h"   // openConsoleWithReturn, settingsCycleTimeZone, resetSettingsNav (currently lives there)
#include "graphics.h"       // ui_showMessage
#include "wifi_time.h"      // wifiIsEnabled, wifiSetEnabled

namespace UiSettingsPages {
  void Handle_MAIN(InputState& input, int move) {
    (void)input; (void)move;
    // TODO: move main settings page logic here
  }
}