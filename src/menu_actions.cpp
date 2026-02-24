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

static bool g_namePetJustOpened = false;

// Shop helper (from shop_actions.cpp)
void shopBuyItem();

// ------------------------------------------------------------------
// Local prototypes
// ------------------------------------------------------------------
void handleSleepScreenInput(InputState &input);
void handleWifiSetupInput(InputState &in);

void handlePetScreen(const InputState &input);
void handleInventoryInput(const InputState &input);
void handleShopInput(const InputState &input);
void handleSleepMenuInput(const InputState &input);
void handleTimeSetInput(InputState &in);
void handleMiniGameInput(const InputState &input);
void handleDeathInput(InputState &input);
void handlePowerMenuInput(InputState &input);
void handleChoosePetInput(InputState &in);
void handleNamePetInput(InputState &in);
void handleBurialInput(InputState &in);
static void beginNamePetFlow_local();
static void finalizeNewPetFromName_local(InputState &in);
static void useFoodAction();
static uint32_t g_suppressMenuUntilMs = 0;
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

// After leaving certain screens (like Console), the same ESC/MENU press can
// still be observed on the next frame and trigger global settings-open logic.
// This suppresses menu/esc edges for a short time window.

static inline bool menuSuppressedNow() {
  // signed diff to handle millis wrap safely
  return (int32_t)(millis() - g_suppressMenuUntilMs) < 0;
}

void menuActionsSuppressMenuForMs(uint32_t durationMs) {
  g_suppressMenuUntilMs = millis() + durationMs;
}

// After returning to PET_SLEEPING from overlays (Power Menu),
// suppress wake edges briefly so the exit key doesn't immediately wake the pet.
static uint32_t g_suppressSleepWakeUntilMs = 0;

// NEW: pending pet type through the new-pet flow (egg -> name)
static PetType g_pendingPetType = PET_DEVIL;

// ==================================================================
// TIME ZONE: cycle/apply/persist immediately
// ==================================================================
void settingsCycleTimeZone(int dir) {
  const int count = (int)tzCount();
  if (count <= 0) return;

  // wrap-safe
  int next = (int)tzIndex + dir;
  while (next < 0) next += count;
  next %= count;

  tzIndex = (uint8_t)next;

  // Apply immediately so localtime_r() and UI hour are correct right away
  applyTimezoneIndex(tzIndex);

  // Force a HUD recompute immediately (prevents “old hour” until next tick)
  updateTime();

  // Persist immediately even if the game isn't "dirty"
  saveSettingsToSD();
  saveTzIndexToNVS(tzIndex);

  uint8_t rb = 255;
  if (loadTzIndexFromNVS(&rb)) {
    DBG_ON("[TZ] saved idx=%u readback=%u\n", (unsigned)tzIndex, (unsigned)rb);
  } else {
    DBG_ON("[TZ] saved idx=%u readback FAILED\n", (unsigned)tzIndex);
  }

  requestUIRedraw();
  playBeep();

  // Prevent double-advances / extra key effects
  clearInputLatch();
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
bool handleMenuInput(InputState &in) {
  // --------------------------------------------------------------
  // BOOT FIXUP:
  // Only apply this when we are truly in a bad boot resume.
  // DO NOT run this during a legitimate new-pet flow (egg->hatch->name).
  // --------------------------------------------------------------
  if (!g_bootNamePetFixApplied && g_app.uiState == UIState::NAME_PET) {
    g_bootNamePetFixApplied = true;

    // If we're actively in the new-pet flow, NAME_PET is valid.
    if (g_app.newPetFlowActive) {
      swallowTypingAndEdges(in);
      return true;
    }

    // Otherwise, treat NAME_PET as suspicious only if we are not text-capturing.
    // (Bad resume tends to land here without the keyboard mode enabled.)
    if (!g_textCaptureMode) {
      inputSetTextCapture(false);
      g_textCaptureMode = false;

      g_app.uiState    = UIState::CHOOSE_PET;
      g_app.currentTab = Tab::TAB_PET;

      g_choosePetBlockHatchUntilRelease = true;

      requestUIRedraw();
      invalidateBackgroundCache();
      requestUIRedraw();

      swallowTypingAndEdges(in);
      inputForceClear();
      return true;
    }
  }

  UIState oldState = g_app.uiState;

  if (menuSuppressedNow() && (in.menuOnce || in.escOnce)) {
    swallowTypingAndEdges(in);
    return false;
  }

  // --------------------------------------------------------------
  // PET_SCREEN: ESC should open Settings (classic behavior)
  // --------------------------------------------------------------
  if ((g_app.uiState == UIState::PET_SCREEN ||
       g_app.uiState == UIState::INVENTORY ||
       g_app.uiState == UIState::SHOP ||
       g_app.uiState == UIState::PET_SLEEPING ||
       g_app.uiState == UIState::SLEEP_MENU) &&
      (in.escOnce || in.hotSettings)) {

    g_settingsFlow.settingsReturnState = g_app.uiState;
    g_settingsFlow.settingsReturnTab   = g_app.currentTab;
    g_settingsFlow.settingsReturnValid = true;

    resetSettingsNav(true);
    g_settingsFlow.settingsPage = SettingsPage::TOP;

    g_app.uiState = UIState::SETTINGS;
    requestUIRedraw();

    drainKb(in);
    inputForceClear();

    in.escOnce          = false;
    in.menuOnce         = false;
    in.selectOnce       = false;
    in.encoderPressOnce = false;

    return true;
  }

  // --------------------------------------------------------------
  // HARD INPUT GATE WHILE SLEEPING
  //  - ENTER wakes (select/encoderPress)
  //  - ESC opens Settings WITHOUT waking
  //  - EVERYTHING ELSE is swallowed
  // --------------------------------------------------------------
  if (g_app.uiState == UIState::PET_SLEEPING) {
    // Use a HELD->edge for wake because selectOnce depends on Keyboard.isChange().
    static bool s_prevSleepSelectHeld = false;
    const bool enterEdge              = (in.selectHeld && !s_prevSleepSelectHeld);
    s_prevSleepSelectHeld             = in.selectHeld;

    const bool wakePressed = (enterEdge || in.encoderPressOnce || in.selectOnce);

    if (in.escOnce && !wakePressed) {
      resetSettingsNav(true);
      g_settingsFlow.settingsPage = SettingsPage::TOP;
      g_app.uiState               = UIState::SETTINGS;
      requestUIRedraw();

      drainKb(in);
      inputForceClear();

      in.escOnce  = false;
      in.menuOnce = false;

      return true;
    }

    if (!wakePressed) {
      // Swallow everything without calling clearInputLatch() (it can erase wake edges).
      drainKb(in);
      in.clearEdges();
      return false;
    }
  }

  uiDispatchToStateHandler(g_app.uiState, in);

  return (oldState != g_app.uiState);
}

// ==================================================================
// SLEEP SCREEN ACTIVE
// ==================================================================
void handleSleepScreenInput(InputState &input) {
  // Suppress immediate wake after returning from power menu
  if (powerMenuSleepWakeSuppressedNow()) {
  drainKb(input);
    input.clearEdges();
    return;
  }

  // Wake detection: use selectHeld rising edge (robust even if isChange() misses)
  static bool s_prevSelectHeld = false;
  const bool enterEdge         = (input.selectHeld && !s_prevSelectHeld);
  s_prevSelectHeld             = input.selectHeld;

  if (enterEdge || input.encoderPressOnce || input.selectOnce) {
    pet.isSleeping  = false;
    g_app.isSleeping      = false;
    g_app.sleepingByTimer = false;

    saveManagerMarkDirty();

    g_app.uiState    = UIState::PET_SCREEN;
    g_app.currentTab = Tab::TAB_PET;
    requestUIRedraw();

    g_app.sleepUntilRested   = false;
    g_app.sleepUntilAwakened = false;
    g_app.sleepTargetEnergy  = 0;

    swallowTypingAndEdges(input);
    return;
  }

  if (input.escOnce) {
    resetSettingsNav(true);
    g_settingsFlow.settingsPage = SettingsPage::TOP;
    g_app.uiState               = UIState::SETTINGS;
    requestUIRedraw();

    drainKb(input);
    inputForceClear();

    input.escOnce  = false;
    input.menuOnce = false;
    return;
  }

  // Otherwise swallow
  drainKb(input);
  input.clearEdges();
}

// ==================================================================
// PET SCREEN (main hub)
// ==================================================================
void handlePetScreen(const InputState &input) {
  if (g_app.currentTab == Tab::TAB_FEED) {
    int mv = 0;
    if (input.leftOnce || input.upOnce || (input.encoderDelta < 0)) mv = -1;
    if (input.rightOnce || input.downOnce || (input.encoderDelta > 0)) mv = +1;

    if (mv != 0) {
      feedMenuIndex += mv;
      if (feedMenuIndex < 0) feedMenuIndex = 1;
      if (feedMenuIndex > 1) feedMenuIndex = 0;

      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      feedUseSelected();
      clearInputLatch();
    }
    return;
  }

  if (g_app.currentTab == Tab::TAB_PLAY) {
    // Play tab games (keep in sync with drawPlayTabMock() labels)
    static const int kPlayItems = 3;

    int mv = 0;
    if (input.leftOnce || input.upOnce || (input.encoderDelta < 0)) mv = -1;
    if (input.rightOnce || input.downOnce || (input.encoderDelta > 0)) mv = +1;

    if (mv != 0) {
      playMenuIndex += mv;
      while (playMenuIndex < 0) playMenuIndex += kPlayItems;
      playMenuIndex %= kPlayItems;

      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      // Cost to play: 10 energy (regular Play tab only)
      if (pet.energy < 10) {
        ui_showMessage("Too tired!");
        soundError();
        clearInputLatch();
        return;
      }

      pet.energy = constrain(pet.energy - 10, 0, 100);
      saveManagerMarkDirty();

      // Launch selected game
      if (playMenuIndex == 0) {
        startFlappyFireball();
      } else if (playMenuIndex == 1) {
        startInfernalDodger();
      } else {
        startCrossyRoad();
      }

      clearInputLatch();
      return;
    }

    return;
  }
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
// INVENTORY
// ==================================================================
void handleInventoryInput(const InputState &input) {
  if (input.menuOnce) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  int count = g_app.inventory.countItems();
  if (count == 0) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  int move = input.encoderDelta;
  if (input.upOnce) move = -1;
  if (input.downOnce) move = 1;

  if (move != 0) {
    g_app.inventory.selectedIndex += move;
    if (g_app.inventory.selectedIndex < 0) g_app.inventory.selectedIndex = count - 1;
    if (g_app.inventory.selectedIndex >= count) g_app.inventory.selectedIndex = 0;

    requestUIRedraw();
    playBeep();
    return;
  }

  if (input.selectOnce || input.encoderPressOnce) {
    g_app.inventory.useSelectedItem();
    saveManagerMarkDirty();
    requestUIRedraw();
    clearInputLatch();
  }
}

// ==================================================================
// SHOP MENU
// ==================================================================
void handleShopInput(const InputState &input) {
  if (input.menuOnce) {
    g_app.uiState = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  int move = input.encoderDelta;
  if (input.upOnce) move = -1;
  if (input.downOnce) move = 1;

  if (move != 0) {
    const int totalItems = SHOP_ITEM_COUNT; // no Exit pill
    if (totalItems > 0) {
      // True round-robin, supports move magnitudes > 1
      shopIndex = (shopIndex + move) % totalItems;
      if (shopIndex < 0) shopIndex += totalItems;

      requestUIRedraw();
      playBeep();
    }
    return;
  }

  if (input.selectOnce || input.encoderPressOnce) {
    shopBuyItem();
    requestUIRedraw();
    clearInputLatch();
  }
}

// ==================================================================
// SLEEP MENU
// ==================================================================
void handleSleepMenuInput(const InputState &input) {
  const int totalItems = 4;

  if (input.encoderDelta < 0 || input.upOnce) {
    sleepMenuIndex--;
    if (sleepMenuIndex < 0) sleepMenuIndex = totalItems - 1;
    requestUIRedraw();
  }

  if (input.encoderDelta > 0 || input.downOnce) {
    sleepMenuIndex++;
    if (sleepMenuIndex >= totalItems) sleepMenuIndex = 0;
    requestUIRedraw();
  }

  if (input.menuOnce || input.escOnce) {
    g_app.uiState    = UIState::PET_SCREEN;
    g_app.currentTab = Tab::TAB_PET;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  if (!(input.selectOnce || input.encoderPressOnce)) return;

  auto enterSleep = [&]() {
    pet.isSleeping    = true;
    g_app.isSleeping        = true;
    g_app.uiState     = UIState::PET_SLEEPING;
    g_app.currentTab  = Tab::TAB_PET;
    requestUIRedraw();

    // Prevent the ENTER/encoder press that *started* sleep from immediately waking it.
    g_suppressSleepWakeUntilMs = millis() + 400;

    // Eat any residue so the sleep screen starts "clean"
    inputForceClear();
    clearInputLatch();

    g_app.sleepTargetEnergy = 0;
    invalidateBackgroundCache();
    saveManagerMarkDirty();

    sleepBgKickNow();
  };

  switch (sleepMenuIndex) {
    case 0: // Until Awakened
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = true;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 0;
      enterSleep();
      break;

    case 1: // Until Rested
      g_app.sleepUntilRested   = true;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 0;
      enterSleep();
      break;

    case 2: // 4 hours
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 4UL * 60UL * 60UL * 1000UL;
      enterSleep();
      break;

    case 3: // 8 hours
      g_app.sleepUntilRested   = false;
      g_app.sleepUntilAwakened = false;
      g_app.sleepStartTime     = millis();
      g_app.sleepDurationMs    = 8UL * 60UL * 60UL * 1000UL;
      enterSleep();
      break;
  }

  clearInputLatch();
}

// ==================================================================
// MINI GAME
// ==================================================================
void handleMiniGameInput(const InputState &input) {
  if (input.menuOnce) {
    g_app.inMiniGame    = false;
    currentMiniGame     = MiniGame::NONE;
    g_app.uiState       = UIState::PET_SCREEN;
    requestUIRedraw();
    clearInputLatch();
    return;
  }

  updateMiniGame(input);
  requestUIRedraw();
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
// WIFI SETUP WIZARD INPUT
// ==================================================================
void handleWifiSetupInput(InputState &in) {
  // Cancel
  if (in.menuOnce || in.escOnce) {
    inputSetTextCapture(false);
    g_textCaptureMode = false;

    if (g_wifiSetupFromBootWizard) {
      g_app.uiState = UIState::BOOT_WIFI_PROMPT;
      requestUIRedraw();
      g_wifiSetupFromBootWizard = false;

      drainKb(in);
      clearInputLatch();
      return;
    }

    g_app.uiState               = UIState::SETTINGS;
    g_settingsFlow.settingsPage = SettingsPage::WIFI;
    ui_showMessage("Cancelled");
    requestUIRedraw();

    drainKb(in);
    clearInputLatch();
    return;
  }

  // Key input
  while (in.kbHasEvent()) {
    KeyEvent e = in.kbPop();
    uint8_t code = e.code;

    const bool isEnter =
      (code == (uint8_t)KEY_ENTER) || (code == '\n') || (code == '\r') || (code == 0x28);

    const bool isBackspace =
      (code == (uint8_t)KEY_BACKSPACE) || (code == '\b') || (code == 127) || (code == 0x2A);

    if (isBackspace) {
      size_t len = strnlen(wifiSetupBuf, sizeof(wifiSetupBuf));
      if (len > 0) wifiSetupBuf[len - 1] = '\0';
      requestUIRedraw();
      continue;
    }

    if (isEnter) {
      if (g_wifi.setupStage == 0) {
        strncpy(wifiSetupSsid, wifiSetupBuf, sizeof(wifiSetupSsid) - 1);
        wifiSetupSsid[sizeof(wifiSetupSsid) - 1] = '\0';

        g_wifi.setupStage = 1;
        wifiSetupBuf[0]     = '\0';
        requestUIRedraw();

        clearInputLatch();
      } else {
        strncpy(wifiSetupPass, wifiSetupBuf, sizeof(wifiSetupPass) - 1);
        wifiSetupPass[sizeof(wifiSetupPass) - 1] = '\0';

        wifiStoreSave(String(wifiSetupSsid), String(wifiSetupPass));

        wifiSetEnabled(true);
        applyWifiPower(true);
        settingsSetWifiEnabled(true);

        wifiTimeInit();

        saveSettingsToSD();
        saveManagerMarkDirty();

        inputSetTextCapture(false);
        g_textCaptureMode = false;

        if (g_wifiSetupFromBootWizard) {
          g_wifiSetupFromBootWizard = false;
          g_app.uiState = UIState::BOOT_WIFI_WAIT;
          requestUIRedraw();

          clearInputLatch();
          return;
        }

        g_app.uiState               = UIState::SETTINGS;
        g_settingsFlow.settingsPage = SettingsPage::WIFI;
        ui_showMessage("WiFi saved");
        requestUIRedraw();

        clearInputLatch();
      }
      continue;
    }

    if (code >= 32 && code <= 126) {
      size_t len = strnlen(wifiSetupBuf, sizeof(wifiSetupBuf));
      if (len < sizeof(wifiSetupBuf) - 1) {
        wifiSetupBuf[len]     = (char)code;
        wifiSetupBuf[len + 1] = '\0';
        requestUIRedraw();
      }
    }
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

// ==================================================================
// DEATH INPUT
// ==================================================================
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

void handleBurialInput(InputState &in) {
  if (!(in.selectOnce || in.encoderPressOnce)) {
    drainKb(in);
    clearInputLatch();
    return;
  }

  // Show the toast and FORCE a render pass so it actually appears before reboot.
  ui_showMessage("Rest in peace...");
  forceRenderUIOnce();

  // Let the player actually see it (ui_showMessage toast duration is ~900ms).
  delay(950);

  soundResetDeathDirgeLatch();

  saveManagerDeletePetOnly();

  // Small settle delay for SD removes (optional but harmless)
  delay(50);

  ESP.restart();
}
