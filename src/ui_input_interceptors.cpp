#include "ui_input_interceptors.h"

#include <Arduino.h>

#include "M5Cardputer.h"   // <-- direct keyboard probe fallback

#include "app_state.h"
#include "input.h"

#include "graphics.h"
#include "settings_flow_state.h"
#include "ui_runtime.h"
#include "ui_suppress.h"
#include "ui_input_common.h"

#include "ui_actions.h"
#include "ui_state_console.h"
#include "flow_power_menu.h"

#include "save_manager.h"  // saveManagerGetBirthEpoch()

// Keep the boot fix local to this module.
static bool s_bootNamePetFixApplied = false;

// Menu/ESC suppression is centralized in ui_suppress.*
static inline bool menuSuppressedNow() { return uiIsMenuSuppressed(); }

// -----------------------------------------------------------------------------
// Direct ESC probe (fallback for tabs where InputState::escOnce isn't generated)
// -----------------------------------------------------------------------------
static bool s_escPhysLatched = false;

static inline bool escPhysHeld()
{
  // Cardputer "ESC" is commonly the ` / ~ key on many firmwares/layouts.
  // Also try ASCII ESC (0x1B). Some firmwares might expose 0x29 (HID ESC)
  // through isKeyPressed(char) depending on implementation.
  if (M5Cardputer.Keyboard.isKeyPressed('`')) return true;
  if (M5Cardputer.Keyboard.isKeyPressed('~')) return true;
  if (M5Cardputer.Keyboard.isKeyPressed((char)0x1B)) return true; // ASCII ESC
  if (M5Cardputer.Keyboard.isKeyPressed((char)0x29)) return true; // best-effort

  // Some layouts have been observed to emit a degree-like glyph in word stream.
  // We can probe it as a held char too.
  if (M5Cardputer.Keyboard.isKeyPressed((char)0xB0)) return true;

  return false;
}

static inline void synthesizeEscOnceIfNeeded(InputState& in)
{
  // If your input layer already delivered escOnce, don't interfere.
  if (in.escOnce)
  {
    s_escPhysLatched = true;
    return;
  }

  const bool held = escPhysHeld();
  if (held)
  {
    if (!s_escPhysLatched)
    {
      in.escOnce = true;
      // Optional: keep a breadcrumb so you can see it firing even in "affected tabs"
      Serial.printf("[IN] ESC synth ui=%d tab=%d\n", (int)g_app.uiState, (int)g_app.currentTab);
      s_escPhysLatched = true;
    }
  }
  else
  {
    s_escPhysLatched = false;
  }
}

// MENU/Q allowed states (same as your original intent)
static inline bool canOpenSettingsFrom(UIState s)
{
  switch (s)
  {
    case UIState::PET_SCREEN:
    case UIState::INVENTORY:
    case UIState::SHOP:
    case UIState::SLEEP_MENU:
      return true;
    default:
      return false;
  }
}

// ESC should open settings broadly, but must not steal from modal/text entry states.
// NOTE: ALSO exclude SETTINGS so ESC can dismiss it normally.
static inline bool escCanOpenSettingsFrom(UIState s)
{
  switch (s)
  {
    case UIState::CONSOLE:      // handled earlier (closeConsoleAndReturn)
    case UIState::POWER_MENU:   // handled earlier (powerMenuClose)
    case UIState::SETTINGS:     // IMPORTANT: let settings handler dismiss itself

    case UIState::SET_TIME:
    case UIState::NAME_PET:
    case UIState::WIFI_SETUP:
    case UIState::CHOOSE_PET:

    case UIState::HATCHING:
    case UIState::EVOLUTION:
    case UIState::MINI_GAME:
    case UIState::DEATH:
    case UIState::BURIAL_SCREEN:
    case UIState::PET_SLEEPING:
      return false;

    default:
      return true;
  }
}

static void openSettingsFromHere(InputState& in)
{
  g_settingsFlow.settingsReturnState = g_app.uiState;
  g_settingsFlow.settingsReturnTab   = g_app.currentTab;

  uiActionEnterState(UIState::SETTINGS, g_app.currentTab, true);

  // Eat edges so they don't double-trigger inside settings
  in.escOnce  = false;
  in.menuOnce = false;

  requestUIRedraw();
  uiDrainKb(in);
  inputForceClear();
  clearInputLatch();
}

static bool handleConsoleExit(InputState& in)
{
  if (g_app.uiState != UIState::CONSOLE)
    return false;

  if (in.escOnce || in.menuOnce)
    return closeConsoleAndReturn(in);

  return false;
}

static bool handlePowerMenuExit(InputState& in)
{
  if (g_app.uiState != UIState::POWER_MENU)
    return false;

  if (in.escOnce || in.menuOnce)
  {
    powerMenuClose();

    in.escOnce  = false;
    in.menuOnce = false;

    requestUIRedraw();
    uiDrainKb(in);
    inputForceClear();
    clearInputLatch();
    return true;
  }

  return false;
}

static bool handleBootNamePetFix(InputState& in)
{
  // One-time fix: if we ever re-enter NAME_PET after boot with a pet already alive,
  // bounce back to pet screen.
  if (s_bootNamePetFixApplied)
    return false;

  if (g_app.uiState != UIState::NAME_PET)
    return false;

  // Use save manager birth epoch (no incomplete-type Pet access)
  if (saveManagerGetBirthEpoch() == 0)
    return false;

  s_bootNamePetFixApplied = true;

  uiActionEnterState(UIState::PET_SCREEN, Tab::TAB_PET, true);
  in.clearEdges();

  requestUIRedraw();
  uiDrainKb(in);
  inputForceClear();
  clearInputLatch();
  return true;
}

static bool handleMenuSuppression(InputState& in)
{
  if (!menuSuppressedNow())
    return false;

  // While suppressed, swallow menu/esc edges (and console toggle) so we don't re-trigger.
  if (in.menuOnce || in.escOnce || in.consoleOnce)
  {
    in.menuOnce = false;
    in.escOnce = false;
    in.consoleOnce = false;
    return true; // consumed
  }

  return false;
}

bool uiInputApplyInterceptors(InputState& in)
{
  // 0) Ensure ESC exists even in tabs where the input layer fails to generate it.
  // Do this BEFORE suppression logic so it participates consistently.
  synthesizeEscOnceIfNeeded(in);

  // 1) If menu is suppressed, swallow edges and stop.
  if (handleMenuSuppression(in))
    return true;

  // 2) Console dismissal priority (ESC/MENU closes console).
  if (handleConsoleExit(in))
    return true;

  // 3) Power menu dismissal (ESC/MENU closes power menu).
  if (handlePowerMenuExit(in))
    return true;

  // 4) Boot fixups.
  if (handleBootNamePetFix(in))
    return true;

  // 5) If already in Settings, do NOT intercept ESC/MENU here.
  // Let the Settings handler own dismissal.
  if (g_app.uiState == UIState::SETTINGS)
    return false;

  // 6) ESC opens settings broadly (except modal/text-entry states).
  if (in.escOnce && escCanOpenSettingsFrom(g_app.uiState))
  {
    openSettingsFromHere(in);
    return true;
  }

  // 7) MENU/Q opens settings only from allowed states.
  if (in.menuOnce && canOpenSettingsFrom(g_app.uiState))
  {
    openSettingsFromHere(in);
    return true;
  }

  return false;
}

bool uiHandleGlobalInterceptors(InputState& in)
{
  return uiInputApplyInterceptors(in);
}