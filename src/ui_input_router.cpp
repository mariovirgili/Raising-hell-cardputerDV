#include "ui_input_router.h"

#include <Arduino.h>

#include "app_state.h"
#include "flow_power_menu.h"
#include "graphics.h"
#include "input.h"
#include "menu_actions_internal.h"
#include "new_pet_flow_state.h"
#include "settings_flow_state.h"
#include "ui_menu_state.h"
#include "ui_runtime.h"
#include "ui_state_handlers.h"
#include "settings_nav_state.h"
#include "flow_power_menu.h"

// --------------------------------------------------------------
// Local helpers (kept private to the router)
// --------------------------------------------------------------
static inline void drainKb(InputState &in)
{
  while (in.kbHasEvent())
    (void)in.kbPop();
}

static inline void swallowTypingAndEdges(InputState &in)
{
  drainKb(in);
  in.clearEdges();
  clearInputLatch();
}

// --------------------------------------------------------------
// Menu/ESC suppression window (used by overlays so an ESC press
// doesn't immediately re-open Settings on the next tick).
// --------------------------------------------------------------
static uint32_t s_suppressMenuUntilMs = 0;

static inline bool menuSuppressedNow()
{
  // signed diff to handle millis wrap safely
  return (int32_t)(millis() - s_suppressMenuUntilMs) < 0;
}

void menuActionsSuppressMenuForMs(uint32_t durationMs)
{
  s_suppressMenuUntilMs = millis() + durationMs;
}

// --------------------------------------------------------------
// Boot fixup guard
// --------------------------------------------------------------
static bool s_bootNamePetFixApplied = false;

// --------------------------------------------------------------
// Global interceptors
// Return true if handled (and input is swallowed/mutated accordingly)
// --------------------------------------------------------------
static bool handleGlobalInterceptors(InputState &in)
{
  // --------------------------------------------------------------
  // POWER MENU OVERLAY (highest priority)
  //  - When open, it consumes ALL input.
  //  - Opens via GO long-hold (below).
  // --------------------------------------------------------------
  if (g_app.uiState == UIState::POWER_MENU)
  {
    uiPowerMenuHandle(in);
    return true;
  }

  // Open power menu ONLY via GO long-hold (do not tie to menuOnce/escOnce).
  if (!g_textCaptureMode && in.goLongHold)
  {
    openPowerMenuFromHere(millis());
    swallowTypingAndEdges(in);
    return true;
  }

  // --------------------------------------------------------------
  // BOOT FIXUP:
  // Only apply this when we are truly in a bad boot resume.
  // DO NOT run this during a legitimate new-pet flow (egg->hatch->name).
  // --------------------------------------------------------------
  if (!s_bootNamePetFixApplied && g_app.uiState == UIState::NAME_PET)
  {
    s_bootNamePetFixApplied = true;

    // If we're actively in the new-pet flow, NAME_PET is valid.
    if (g_app.newPetFlowActive)
    {
      swallowTypingAndEdges(in);
      return true;
    }

    // Otherwise, treat NAME_PET as suspicious only if we are not text-capturing.
    if (!g_textCaptureMode)
    {
      inputSetTextCapture(false);
      g_textCaptureMode = false;

      g_app.uiState = UIState::CHOOSE_PET;
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

  // --------------------------------------------------------------
  // Suppress menu/esc edges for a brief window after leaving overlays.
  // --------------------------------------------------------------
  if (menuSuppressedNow() && (in.menuOnce || in.escOnce))
  {
    swallowTypingAndEdges(in);
    return true; // swallow AND stop dispatch this tick
  }

  // --------------------------------------------------------------
  // PET_SCREEN + main tabs: ESC should open Settings (classic behavior)
  // --------------------------------------------------------------
  if ((g_app.uiState == UIState::PET_SCREEN || g_app.uiState == UIState::INVENTORY || g_app.uiState == UIState::SHOP ||
       g_app.uiState == UIState::PET_SLEEPING || g_app.uiState == UIState::SLEEP_MENU) &&
      (in.escOnce || in.hotSettings))
  {
    g_settingsFlow.settingsReturnState = g_app.uiState;
    g_settingsFlow.settingsReturnTab = g_app.currentTab;
    g_settingsFlow.settingsReturnValid = true;

    resetSettingsNav(true);
    g_settingsFlow.settingsPage = SettingsPage::TOP;

    g_app.uiState = UIState::SETTINGS;
    requestUIRedraw();

    drainKb(in);
    inputForceClear();

    in.escOnce = false;
    in.menuOnce = false;
    in.selectOnce = false;
    in.encoderPressOnce = false;

    return true;
  }

  // --------------------------------------------------------------
  // HARD INPUT GATE WHILE SLEEPING
  //  - ENTER wakes (select/encoderPress)
  //  - ESC opens Settings WITHOUT waking
  //  - EVERYTHING ELSE is swallowed
  // --------------------------------------------------------------
  if (g_app.uiState == UIState::PET_SLEEPING)
  {
    static bool s_prevSleepSelectHeld = false;
    const bool enterEdge = (in.selectHeld && !s_prevSleepSelectHeld);
    s_prevSleepSelectHeld = in.selectHeld;

    const bool wakePressed = (enterEdge || in.encoderPressOnce || in.selectOnce);

    if (in.escOnce && !wakePressed)
    {
      resetSettingsNav(true);
      g_settingsFlow.settingsPage = SettingsPage::TOP;
      g_app.uiState = UIState::SETTINGS;
      requestUIRedraw();

      drainKb(in);
      inputForceClear();

      in.escOnce = false;
      in.menuOnce = false;

      return true;
    }

    if (!wakePressed)
    {
      drainKb(in);
      in.clearEdges();
      return true; // swallow AND stop dispatch
    }
  }

  return false;
}

bool uiHandleInput(InputState &in)
{
  if (handleGlobalInterceptors(in))
    return true;

  return uiDispatchToStateHandler(g_app.uiState, in);
}