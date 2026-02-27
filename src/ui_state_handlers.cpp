#include "ui_state_handlers.h"

#include "ui_defs.h"

// Forward declarations (implemented across your ui_state_* and flow_* modules)
void uiTabDrivenHandle(InputState&);

void uiNamePetHandle(InputState&);
void uiPetScreenHandle(InputState&);
void uiPetSleepingHandle(InputState&);
void uiInventoryHandle(InputState&);
void uiShopHandle(InputState&);
void uiSettingsHandle(InputState&);
void uiSleepMenuHandle(InputState&);
void uiConsoleHandle(InputState&);
void uiPowerMenuHandle(InputState&);
void uiMiniGameHandle(InputState&);
void uiDeathHandle(InputState&);
void uiBurialScreenHandle(InputState&);

// Flows that already provide handlers (not necessarily in ui_state_* files)
void uiWifiSetupHandle(InputState&);
void uiChoosePetHandle(InputState&);
void uiSetTimeHandle(InputState&);
void uiControlsHelpHandle(InputState&);

// Boot flow handlers
void uiBootWifiPromptHandle(InputState&);
void uiBootWifiWaitHandle(InputState&);
void uiBootTzPickHandle(InputState&);
void uiBootNtpWaitHandle(InputState&);

// Hatching/Evolution flows
void uiHatchingHandle(InputState&);
void uiEvolutionHandle(InputState&);

// Some states intentionally have no input (BOOT) or are handled elsewhere.
static void uiNoopHandle(InputState&) {}

static constexpr int kUIStateCount = (int)UIState::MG_PAUSE + 1;

static StateHandlerFn kHandlers[kUIStateCount] = {
  /*  0 BOOT             */ uiNoopHandle,
  /*  1 HOME             */ uiTabDrivenHandle,
  /*  2 POWER_MENU       */ uiPowerMenuHandle,
  /*  3 PET_SCREEN       */ uiPetScreenHandle,
  /*  4 PET_SLEEPING     */ uiPetSleepingHandle,
  /*  5 INVENTORY        */ uiInventoryHandle,
  /*  6 SHOP             */ uiShopHandle,
  /*  7 CONSOLE          */ uiConsoleHandle,
  /*  8 DEATH            */ uiDeathHandle,
  /*  9 BURIAL           */ uiBurialScreenHandle,
  /* 10 WIFI_SETUP       */ uiWifiSetupHandle,
  /* 11 CHOOSE_PET       */ uiChoosePetHandle,
  /* 12 SETTINGS         */ uiSettingsHandle,
  /* 13 SLEEP_MENU       */ uiSleepMenuHandle,
  /* 14 SET_TIME         */ uiSetTimeHandle,
  /* 15 HATCHING         */ uiHatchingHandle,
  /* 16 CONTROLS_HELP    */ uiControlsHelpHandle,
  /* 17 BOOT_WIFI_PROMPT */ uiBootWifiPromptHandle,
  /* 18 BOOT_WIFI_WAIT   */ uiBootWifiWaitHandle,
  /* 19 BOOT_TZ_PICK     */ uiBootTzPickHandle,
  /* 20 BOOT_NTP_WAIT    */ uiBootNtpWaitHandle,
  /* 21 EVOLUTION        */ uiEvolutionHandle,
  /* 22 MG_PAUSE         */ uiMiniGameHandle, // Your pause overlay is handled inside uiMiniGameHandle/mgPause*
};

bool uiDispatchToStateHandler(UIState state, InputState& in)
{
  const int idx = (int)state;
  if (idx < 0 || idx >= kUIStateCount) return false;

  StateHandlerFn fn = kHandlers[idx];
  if (!fn) return false;

  fn(in);
  return true;
}