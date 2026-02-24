#include "ui_settings_menu.h"

#include "app_state.h"
#include "settings_flow_state.h"
#include "ui_runtime.h"
#include "sound.h"
#include "sdcard.h"
#include "save_manager.h"

#include "display.h"
#include "auto_screen.h"
#include "brightness_state.h"

#include "wifi_power.h"
#include "wifi_store.h"
#include "wifi_setup_state.h"
#include "wifi_time.h"

#include "flow_controls_help.h"
#include "menu_actions.h"   // openConsoleWithReturn, settingsCycleTimeZone
#include "graphics.h"       // ui_showMessage
#include "ui_input_utils.h" // uiDrainKb
#include "game_options_state.h"

// ------------------------------------------------------------
// Minimal embedded menu model
// ------------------------------------------------------------
struct MenuItem {
  const char* label;

  void (*onSelect)(InputState&);
  void (*onLeft)(InputState&);
  void (*onRight)(InputState&);

  bool (*isEnabled)();
};

struct MenuPageDef {
  SettingsPage page;
  MenuItem*    items;
  uint8_t      itemCount;

  // Cursor accessor per page (keeps your existing indices in g_app / g_wifi)
  int& (*cursor)();
};

static int& cursorTop()    { return g_app.settingsIndex; }
static int& cursorScreen() { return g_app.screenSettingsIndex; }
static int& cursorWifi()   { return g_wifi.wifiSettingsIndex; }
static int& cursorGame()   { return g_app.gameOptionsIndex; }

// ------------------------------------------------------------
// Common helpers
// ------------------------------------------------------------
static inline void wrapMove(int& idx, int count, int move)
{
  if (count <= 0) { idx = 0; return; }
  idx += move;
  while (idx < 0) idx += count;
  while (idx >= count) idx -= count;
}

// ------------------------------------------------------------
// TOP page actions
// ------------------------------------------------------------
static void actTop_VolumeSelect(InputState&)
{
  soundAdjustVolume(+1);
  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actTop_VolumeLeft(InputState&)
{
  soundAdjustVolume(-1);
  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actTop_VolumeRight(InputState&)
{
  soundAdjustVolume(+1);
  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actTop_Controls(InputState&)
{
  openControlsHelpFromSettings();
  playBeep();
  clearInputLatch();
}

static void actTop_OpenScreen(InputState&)
{
  g_settingsFlow.settingsPage     = SettingsPage::SCREEN;
  g_app.screenSettingsIndex       = 0;
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actTop_OpenSystem(InputState&)
{
  g_settingsFlow.settingsPage     = SettingsPage::SYSTEM;
  g_app.systemSettingsIndex       = 0;
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actTop_OpenGame(InputState&)
{
  g_settingsFlow.settingsPage     = SettingsPage::GAME;
  g_app.gameOptionsIndex          = 0;
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static bool enConsole()
{
#if PUBLIC_BUILD
  return false;
#else
  return true;
#endif
}

static void actTop_Console(InputState& input)
{
#if PUBLIC_BUILD
  ui_showMessage("Console disabled");
  soundError();
  clearInputLatch();
  (void)input;
#else
  openConsoleWithReturn(UIState::SETTINGS, g_app.currentTab, true, g_settingsFlow.settingsPage);
  uiDrainKb(input);
  requestUIRedraw();
  playBeep();
  clearInputLatch();
#endif
}

static void actTop_Credits(InputState&)
{
  g_settingsFlow.settingsPage = SettingsPage::CREDITS;
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

// ------------------------------------------------------------
// SCREEN page actions
// ------------------------------------------------------------
static void actScreen_BrightnessLeft(InputState&)
{
  brightnessLevel -= 1;
  if (brightnessLevel < 0) brightnessLevel = 2;

  setBacklight((uint16_t)brightnessValues[brightnessLevel]);

  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actScreen_BrightnessRight(InputState&)
{
  brightnessLevel += 1;
  if (brightnessLevel > 2) brightnessLevel = 0;

  setBacklight((uint16_t)brightnessValues[brightnessLevel]);

  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actScreen_AutoScreenSelect(InputState&)
{
  g_app.autoScreenIndex             = (int)autoScreenTimeoutSel;
  g_settingsFlow.settingsReturnPage = SettingsPage::SCREEN;
  g_settingsFlow.settingsPage       = SettingsPage::AUTO_SCREEN;
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

// ------------------------------------------------------------
// WIFI page actions
// ------------------------------------------------------------
static void actWifi_Toggle(InputState&)
{
  const bool en = !wifiIsEnabled();

  wifiSetEnabled(en);
  settingsSetWifiEnabled(en);
  applyWifiPower(en);

  saveSettingsToSD();
  saveManagerMarkDirty();

  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actWifi_SetNetwork(InputState& input)
{
  g_wifiSetupFromBootWizard = false;

  g_wifi.setupStage = 0;
  g_wifi.buf[0]     = '\0';
  g_wifi.ssid[0]    = '\0';
  g_wifi.pass[0]    = '\0';

  g_app.uiState = UIState::WIFI_SETUP;
  requestUIRedraw();

  inputSetTextCapture(true);
  g_textCaptureMode = true;

  uiDrainKb(input);
  playBeep();
  clearInputLatch();
}

static void actWifi_Reset(InputState&)
{
  wifiResetSettings();
  wifiStoreClear();

  ui_showMessage("WiFi reset");
  requestUIRedraw();

  playBeep();
  clearInputLatch();
}

static void actWifi_TzSelect(InputState&)
{
  settingsCycleTimeZone(+1);
  // (settingsCycleTimeZone already redraws + beeps + clears latch)
}

static void actWifi_TzLeft(InputState&)
{
  settingsCycleTimeZone(-1);
}

static void actWifi_TzRight(InputState&)
{
  settingsCycleTimeZone(+1);
}

// ------------------------------------------------------------
// GAME page actions
// ------------------------------------------------------------
static void actGame_DecayMode(InputState&)
{
  g_app.decayModeIndex              = (int)saveManagerGetDecayMode();
  g_settingsFlow.settingsReturnPage = SettingsPage::GAME;
  g_settingsFlow.settingsPage       = SettingsPage::DECAY_MODE;
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actGame_ToggleDeath(InputState&)
{
  petDeathEnabled = !petDeathEnabled;
  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

static void actGame_ToggleLedAlerts(InputState&)
{
  ledAlertsEnabled = !ledAlertsEnabled;
#if LED_STATUS_ENABLED
  ledUpdatePetStatus(LED_PET_OFF);
#endif
  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

// ------------------------------------------------------------
// Menu definitions
// ------------------------------------------------------------
static MenuItem kTopItems[] = {
  { "Volume",   actTop_VolumeSelect,   actTop_VolumeLeft,      actTop_VolumeRight,   nullptr   },
  { "Controls", actTop_Controls,       nullptr,               nullptr,              nullptr   },
  { "Screen",   actTop_OpenScreen,     nullptr,               nullptr,              nullptr   },
  { "System",   actTop_OpenSystem,     nullptr,               nullptr,              nullptr   },
  { "Game",     actTop_OpenGame,       nullptr,               nullptr,              nullptr   },
  { "Console",  actTop_Console,        nullptr,               nullptr,              enConsole  },
  { "Credits",  actTop_Credits,        nullptr,               nullptr,              nullptr   },
};

static MenuItem kScreenItems[] = {
  { "Brightness", nullptr,             actScreen_BrightnessLeft, actScreen_BrightnessRight, nullptr },
  { "Auto Screen", actScreen_AutoScreenSelect, nullptr,          nullptr,                  nullptr },
};

static MenuItem kWifiItems[] = {
  { "WiFi",        actWifi_Toggle,     nullptr,             nullptr,              nullptr },
  { "Set Network", actWifi_SetNetwork, nullptr,             nullptr,              nullptr },
  { "Reset WiFi",  actWifi_Reset,      nullptr,             nullptr,              nullptr },
  { "Time Zone",   actWifi_TzSelect,   actWifi_TzLeft,      actWifi_TzRight,       nullptr },
};

static MenuItem kGameItems[] = {
  { "Decay Mode",  actGame_DecayMode,       nullptr, nullptr, nullptr },
  { "Pet Death",   actGame_ToggleDeath,     nullptr, nullptr, nullptr },
  { "LED Alerts",  actGame_ToggleLedAlerts, nullptr, nullptr, nullptr },
};

static MenuPageDef kPages[] = {
  { SettingsPage::TOP,    kTopItems,    (uint8_t)(sizeof(kTopItems)    / sizeof(kTopItems[0])),    cursorTop    },
  { SettingsPage::SCREEN, kScreenItems, (uint8_t)(sizeof(kScreenItems) / sizeof(kScreenItems[0])), cursorScreen },
  { SettingsPage::WIFI,   kWifiItems,   (uint8_t)(sizeof(kWifiItems)   / sizeof(kWifiItems[0])),   cursorWifi   },
  { SettingsPage::GAME,   kGameItems,   (uint8_t)(sizeof(kGameItems)   / sizeof(kGameItems[0])),   cursorGame   },
};

static const MenuPageDef* findPage(SettingsPage page)
{
  for (uint8_t i = 0; i < (uint8_t)(sizeof(kPages)/sizeof(kPages[0])); ++i) {
    if (kPages[i].page == page) return &kPages[i];
  }
  return nullptr;
}

// ------------------------------------------------------------
// Public entry point
// ------------------------------------------------------------
namespace UiSettingsMenu {

bool Handle(InputState& input, int move)
{
  const MenuPageDef* def = findPage(g_settingsFlow.settingsPage);
  if (!def) return false;

  int& cursor = def->cursor();
  const int count = (int)def->itemCount;

  // Move
  if (move != 0) {
    wrapMove(cursor, count, move);
    requestUIRedraw();
    playBeep();
    return true;
  }

  if (count <= 0) return true;

  // Clamp cursor just in case
  if (cursor < 0) cursor = 0;
  if (cursor >= count) cursor = count - 1;

  MenuItem& item = def->items[cursor];

  // Disabled items: block select/left/right (still allow moving)
  if (item.isEnabled && !item.isEnabled()) {
    if (input.selectOnce || input.encoderPressOnce || input.leftOnce || input.rightOnce) {
      soundError();
      clearInputLatch();
      return true;
    }
    return true;
  }

  // Left/Right
  if (input.leftOnce && item.onLeft)  { item.onLeft(input);  return true; }
  if (input.rightOnce && item.onRight){ item.onRight(input); return true; }

  // Select
  if ((input.selectOnce || input.encoderPressOnce) && item.onSelect) {
    item.onSelect(input);
    return true;
  }

  return true;
}

} // namespace UiSettingsMenu