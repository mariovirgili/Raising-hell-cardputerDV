#include "menu_actions.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <SD.h>

#include "app_state.h"
#include "auto_screen.h"
#include "brightness_state.h"
#include "console.h"
#include "debug.h"
#include "display.h"
#include "eeprom_addrs.h"
#include "feed.h"
#include "game_options_state.h"
#include "graphics.h"
#include "input.h"
#include "input_activity_state.h"
#include "inventory.h"
#include "mini_games.h"
#include "name_entry_state.h"
#include "pet.h"
#include "save_manager.h"
#include "sdcard.h"
#include "shop_items.h"
#include "sleep_state.h"
#include "sound.h"
#include "time_editor_state.h"
#include "time_persist.h"
#include "time_state.h"
#include "timezone.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"
#include "ui_state_handlers.h"
#include "wifi_power.h"
#include "wifi_setup_state.h"
#include "wifi_store.h"
#include "settings_flow_state.h"
#include "flow_power_menu.h"
#include "flow_controls_help.h"
#include "menu_actions_internal.h"
#include "flow_boot_wifi.h"
#include "led_status.h"
#include "flow_factory_reset.h"
#include "flow_time_editor.h"
#include "new_pet_flow_state.h"
#include "build_flags.h"
#include "ui_invalidate.h"
#include "ui_input_router.h"

// Shop helper (from shop_actions.cpp)
void shopBuyItem();

// ------------------------------------------------------------------
// Local prototypes
// ------------------------------------------------------------------
void handleInventoryInput(const InputState &input);
void handleTimeSetInput(InputState &in);
void handleDeathInput(InputState &input);
void handlePowerMenuInput(InputState &input);
void handleChoosePetInput(InputState &in);
void handleNamePetInput(InputState &in);
void handleBurialInput(InputState &in);
static void beginNamePetFlow_local();
static void finalizeNewPetFromName_local(InputState &in);
static void useFoodAction();
static bool g_bootNamePetFixApplied = false;
void resetSettingsNav(bool resetTopIndex);

int inventoryCountType(ItemType type);
static bool consumeOneSoulFood();
static int  consumeSoulFoodUntilFull();
void        feedUseSelected();

static inline void drainKb(InputState &in) {
  while (in.kbHasEvent()) (void)in.kbPop();
}

static inline void swallowTypingAndEdges(InputState &in) {
  drainKb(in);
  in.clearEdges();
  clearInputLatch();
}

// Prefer a *level* signal for Enter held.
static inline bool isEnterHeldLevel(const InputState &in) { return in.selectHeld; }

static inline bool timeSyncedNow() {
  time_t now = time(nullptr);
  return (now > 1700000000); // ~late 2023
}

void handleDeathInput(InputState &in) {
  int move = 0;
  if (in.upOnce) move = -1;
  if (in.downOnce) move = +1;
  if (in.encoderDelta < 0) move = -1;
  if (in.encoderDelta > 0) move = +1;

  if (move != 0) {
    deathMenuIndex += (move > 0) ? 1 : -1;
    if (deathMenuIndex < 0) deathMenuIndex = 1;
    if (deathMenuIndex > 1) deathMenuIndex = 0;

    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  if (!(in.selectOnce || in.encoderPressOnce)) return;

  clearInputLatch();
  inputForceClear();

  if (deathMenuIndex == 0) {
    g_app.inMiniGame = true;
    g_app.gameOver   = false;
    g_app.uiState    = UIState::MINI_GAME;
    requestUIRedraw();

    invalidateBackgroundCache();

    startResurrectionRun();
    currentMiniGame = MiniGame::RESURRECTION;

    inputForceClear();
    clearInputLatch();
    return;

  } else {
    soundResetDeathDirgeLatch();
    soundFuneralDirge();

    g_app.uiState = UIState::BURIAL_SCREEN;
    requestUIRedraw();

    invalidateBackgroundCache();

    inputForceClear();
    clearInputLatch();
    return;
  }
}

// ==================================================================
// NEW PET FLOW: CHOOSE_PET -> NAME_PET
// ==================================================================

static void beginNamePetFlow_local() {
  memset(g_pendingPetName, 0, sizeof(g_pendingPetName));
  inputSetTextCapture(true);
  g_textCaptureMode   = true;
  g_namePetJustOpened = true;

  g_app.uiState = UIState::NAME_PET;
  requestUIRedraw();
  invalidateBackgroundCache();
  requestUIRedraw();
  clearInputLatch();
}

// Finalize new pet immediately after naming.
static void finalizeNewPetFromName_local(InputState &in) {
  inputSetTextCapture(false); // restore normal UI mapping
  g_textCaptureMode = false;

  pet.setName(g_pendingPetName[0] ? g_pendingPetName : "PET");

  // Commit chosen type into the final saved pet
  pet.type = g_pendingPetType;

  g_app.newPetFlowActive = false; // ✅ pet is now real; allow decay/death from here on

  saveManagerMarkDirty();
  saveManagerForce();

  g_app.currentTab = Tab::TAB_PET;
  g_app.uiState    = UIState::PET_SCREEN;
  requestUIRedraw();

  invalidateBackgroundCache();
  requestUIRedraw();
  clearInputLatch();
  playBeep();

  drainKb(in);
}

void handleChoosePetInput(InputState &in) {
  // Reliable Enter edge (press-level -> edge) for Cardputer:
  // Sometimes selectOnce is missed depending on keyboard change detection.
  g_app.newPetFlowActive = true;
  static bool s_prevSelectHeld = false;

  // ---------------------------------------------------------------------------
  // Entry gate to prevent instant hatch
  // ---------------------------------------------------------------------------
  if (g_choosePetBlockHatchUntilRelease) {
    const bool anyPressResidue = (in.selectHeld || in.selectOnce || in.encoderPressOnce);
    if (anyPressResidue) {
      swallowTypingAndEdges(in);
      return;
    }

    // Fully released -> allow normal behavior next tick and seed edge detector
    g_choosePetBlockHatchUntilRelease = false;
    s_prevSelectHeld                 = in.selectHeld;

    swallowTypingAndEdges(in);
    return;
  }

  const bool enterEdge = (in.selectHeld && !s_prevSelectHeld);
  s_prevSelectHeld     = in.selectHeld;

  // LOCKOUT: Backspace/Del may map to menuOnce in UI mode.
  // On egg screen we do NOT want back/menu behavior.
  if (in.escOnce || in.menuOnce) {
    resetSettingsNav(true);
    g_settingsFlow.settingsPage = SettingsPage::TOP;
    g_app.uiState               = UIState::SETTINGS;
    requestUIRedraw();

    swallowTypingAndEdges(in);
    return;
  }

  // Ignore any queued typing junk (we only care about UI mode here)
  drainKb(in);

  // ------------------------------------------------------------
  // Cycle egg selection across all pet types
  // ------------------------------------------------------------
  static const PetType kChoices[] = {
    PET_DEVIL,
    PET_KAIJU,
    PET_ANUBIS,
    PET_AXOLOTL,
    PET_ELDRITCH,
    PET_ALIEN
  };
  static const int kChoiceCount = (int)(sizeof(kChoices) / sizeof(kChoices[0]));

  // Find current index (prefer pending type)
  int curIdx = 0;
  for (int i = 0; i < kChoiceCount; ++i) {
    if (kChoices[i] == g_pendingPetType) { curIdx = i; break; }
  }

  int nextIdx = curIdx;

  // Use left/right or up/down or encoder to switch
  if (in.leftOnce || in.upOnce || (in.encoderDelta < 0)) {
    nextIdx = curIdx - 1;
  } else if (in.rightOnce || in.downOnce || (in.encoderDelta > 0)) {
    nextIdx = curIdx + 1;
  }

  // Wrap
  while (nextIdx < 0) nextIdx += kChoiceCount;
  nextIdx %= kChoiceCount;

  // Decide what the selected pet type should be
#if PUBLIC_BUILD
  const PetType nextType = PET_DEVIL;
#else
  const PetType nextType = kChoices[nextIdx];
#endif

  // Apply selection (only beep/redraw if it actually changed)
  if (nextType != g_pendingPetType) {
    g_pendingPetType = nextType;
    pet.type         = nextType;

    requestUIRedraw();
    invalidateBackgroundCache();
    requestUIRedraw();
    playBeep();
    clearInputLatch();
    return;
  }

  // ------------------------------------------------------------
  // Press ENTER to hatch (robust)
  // ------------------------------------------------------------
  const bool hatchPressed = (enterEdge || in.selectOnce || in.encoderPressOnce);
  if (hatchPressed) {
    // Create the new pet (RAM only). First save happens AFTER naming.
    saveManagerNewPetNoSave();

    // IMPORTANT: preserve the chosen egg type (some new-pet init may reset it)
#if PUBLIC_BUILD
    pet.type         = PET_DEVIL;
    g_pendingPetType = PET_DEVIL; // keep state consistent
#else
    pet.type = g_pendingPetType;
#endif

    // Ensure we're in UI mode (not text capture) for hatching
    inputSetTextCapture(false);

    // Reset hatch state so updateHatching() does one-time init on first tick
    g_app.flow.hatch.active     = false;
    g_app.flow.hatch.startMs    = 0;
    g_app.flow.hatch.frame      = 0;
    g_app.flow.hatch.showingMsg = false;
    g_app.flow.hatch.msgStartMs = 0;

    g_app.uiState = UIState::HATCHING;
    requestUIRedraw();

    invalidateBackgroundCache();
    requestUIRedraw();

    // Consume any residue so the press doesn't leak into the next state
    in.clearEdges();
    inputForceClear();
    clearInputLatch();
    return;
  }
}

void handleNamePetInput(InputState &in) {
  if (g_namePetJustOpened) {
    g_namePetJustOpened = false;
    swallowTypingAndEdges(in);
    requestUIRedraw();
    return;
  }

  // ESC cancels naming (optional: go back to egg)
  if (in.escOnce) {
    inputSetTextCapture(false);
    g_textCaptureMode = false;

    g_app.uiState = UIState::CHOOSE_PET;

    // Prevent the ESC key / stale ENTER from instantly hatching on return
    g_choosePetBlockHatchUntilRelease = true;

    requestUIRedraw();
    invalidateBackgroundCache();
    requestUIRedraw();

    swallowTypingAndEdges(in);
    return;
  }

  bool changed = false;

  while (in.kbHasEvent()) {
    KeyEvent e = in.kbPop();
    const uint8_t c = e.code;

    // Enter can come through as '\n' (most common), sometimes '\r', or KEY_ENTER (0x81)
    if (c == '\n' || c == '\r' || c == KEY_ENTER) {
      if (g_pendingPetName[0] == '\0') { playBeep(); continue; }
      finalizeNewPetFromName_local(in);
      return;
    }

    // Backspace can come through as '\b', sometimes 127, or KEY_BACKSPACE (0x82)
    if (c == '\b' || c == 127 || c == KEY_BACKSPACE) {
      size_t n = strnlen(g_pendingPetName, PET_NAME_MAX);
      if (n > 0) { g_pendingPetName[n - 1] = '\0'; changed = true; }
      continue;
    }

    // Accept printable ASCII only
    if (c >= 32 && c <= 126) {
      size_t n = strnlen(g_pendingPetName, PET_NAME_MAX);
      if (n < PET_NAME_MAX) {
        g_pendingPetName[n]     = (char)c;
        g_pendingPetName[n + 1] = '\0';
        changed = true;
      }
    }
  }

  if (changed) {
    requestUIRedraw();
    requestUIRedraw();
  }
}

// ==================================================================
// MAIN DISPATCHER (state machine)
// ==================================================================
bool handleMenuInput(InputState &in)
{
  const UIState oldState = g_app.uiState;
  uiHandleInput(in);
  return (oldState != g_app.uiState);
}

// ==================================================================
// FEED ACTION (legacy helper; kept)
// ==================================================================
static void useFoodAction() {
  ItemType food = pet.getFoodItem();

  if (!g_app.inventory.hasItem(food)) {
    ui_showMessage("No Food!");
    return;
  }

  g_app.inventory.removeItem(food, 1);

  switch (food) {
    case ITEM_SOUL_FOOD:    pet.hunger    = constrain(pet.hunger + 20, 0, 100); break;
    case ITEM_CURSED_RELIC: pet.happiness = constrain(pet.happiness + 20, 0, 100); break;
    case ITEM_DEMON_BONE:   pet.energy    = constrain(pet.energy + 20, 0, 100); break;
    case ITEM_RITUAL_CHALK: pet.health    = constrain(pet.health + 20, 0, 100); break;
    case ITEM_ELDRITCH_EYE: ui_showMessage("A strange power stirs..."); break;
    default: ui_showMessage("Cannot eat that!"); return;
  }

  pet.lastFedTime = millis();
  saveManagerMarkDirty();
  ui_showMessage("Fed!");
}

// ==================================================================
// SETTINGS MENU
// ==================================================================
void resetSettingsNav(bool resetTopIndex) {
  g_settingsFlow.settingsPage     = SettingsPage::TOP;
  g_app.screenSettingsIndex        = 0;
  g_app.systemSettingsIndex        = 0;
  g_wifi.wifiSettingsIndex          = 0;
  g_app.gameOptionsIndex           = 0;
  g_app.autoScreenIndex            = 0;
  g_app.decayModeIndex             = 0;

  factoryResetResetUiState();

  if (resetTopIndex) g_app.settingsIndex = 0;
}

// ==================================================================
// FEED helpers
// ==================================================================
int inventoryCountType(ItemType type) {
  return g_app.inventory.countType(type);
}

// Apply Soul Food effect + consume exactly one item.
// Returns true only if an item was available and used.
static bool consumeOneSoulFood() {
  if (!g_app.inventory.hasItem(ITEM_SOUL_FOOD)) return false;

  pet.hunger    = constrain(pet.hunger + 20, 0, 100);
  pet.happiness = constrain(pet.happiness + 10, 0, 100);
  pet.energy    = constrain(pet.energy + 10, 0, 100);

  g_app.inventory.removeItem(ITEM_SOUL_FOOD, 1);

  pet.lastFedTime = millis();
  saveManagerMarkDirty();
  return true;
}

static int consumeSoulFoodUntilFull() {
  int used = 0;
  while (pet.hunger < 100) {
    if (!consumeOneSoulFood()) break;
    used++;
    if (used > 99) break; // safety
  }
  return used;
}

void feedUseSelected() {
  if (feedMenuIndex == 0) {
    if (pet.hunger >= 100) {
      ui_showMessage("Already full!");
      requestUIRedraw();
      return;
    }

    if (!consumeOneSoulFood()) ui_showMessage("No Soul Food!");
    else ui_showMessage("Snack time!");

    requestUIRedraw();
    return;
  }

  if (feedMenuIndex == 1) {
    if (pet.hunger >= 100) {
      ui_showMessage("Already full!");
      requestUIRedraw();
      return;
    }

    int used = consumeSoulFoodUntilFull();
    if (used <= 0) ui_showMessage("No Soul Food!");
    else {
      String msg = "Ate x" + String(used);
      ui_showMessage(msg.c_str());
    }

    requestUIRedraw();
    return;
  }
}

// ==================================================================
// RESURRECTION MINI-GAME RESULT (called by mini_games.cpp)
// ==================================================================
void onResurrectionMiniGameResult(bool success) {
  if (success) {
    petResurrectFull();

    currentMiniGame      = MiniGame::NONE;
    g_app.inMiniGame     = false;
    g_app.gameOver             = false;

    g_app.currentTab     = Tab::TAB_PET;
    g_app.uiState        = UIState::PET_SCREEN;
    requestUIRedraw();

    inputForceClear();
    clearInputLatch();

    invalidateBackgroundCache();
    requestUIRedraw();
    return;
  }

  deathMenuIndex        = 0;
  g_app.uiState         = UIState::DEATH;
  requestUIRedraw();

  invalidateBackgroundCache();
  requestUIRedraw();

  inputForceClear();
  clearInputLatch();
}

void settingsToggleLedAlerts() {
  ledAlertsEnabled = !ledAlertsEnabled;

#if LED_STATUS_ENABLED
  if (!ledAlertsEnabled) {
    ledUpdatePetStatus(LED_PET_OFF);
  }
#else
  if (!ledAlertsEnabled) {
    ledUpdatePetStatus(LED_PET_OFF);
  }
#endif

  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}
