#include "ui_input_interceptors.h"

#include <Arduino.h> // millis()

#include "app_state.h"
#include "input.h"

#include "graphics.h"
#include "settings_flow_state.h"
#include "settings_nav_state.h"
#include "ui_defs.h"
#include "ui_runtime.h"
#include "ui_suppress.h"
#include "new_pet_flow_state.h"
#include "ui_input_common.h"
#include "flow_power_menu.h"
#include "factory_reset_state.h"
#include "flow_factory_reset.h"
#include "ui_actions.h"

// Keep the boot fix local to this module.
static bool s_bootNamePetFixApplied = false;

// Menu/ESC suppression is centralized in ui_suppress.*
static inline bool menuSuppressedNow() { return uiIsMenuSuppressed(); }

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

static void openSettingsFromHere(InputState& in)
{
  g_settingsFlow.settingsReturnState = g_app.uiState;
  g_settingsFlow.settingsReturnTab   = g_app.currentTab;
  g_settingsFlow.settingsReturnValid = true;

  resetSettingsNav(true);
  g_settingsFlow.settingsPage = SettingsPage::TOP;

  uiActionEnterState(UIState::SETTINGS, g_app.currentTab, true);

  // Prevent the same ESC/MENU press from being re-consumed immediately.
  uiSuppressMenuForMs(250);
  uiActionSwallowAll(in);
}

// --------------------------------------------------------------
// Interceptor handlers (small, ordered, behavior-preserving)
// --------------------------------------------------------------
static bool handlePowerMenuOverlay(InputState& in)
{
  if (g_app.uiState == UIState::POWER_MENU)
  {
    uiPowerMenuHandle(in);
    return true;
  }
  return false;
}

static bool handlePowerMenuOpen(InputState& in)
{
  // Open power menu ONLY via GO long-hold (do not tie to menuOnce/escOnce).
  if (!g_textCaptureMode && in.goLongHold)
  {
    openPowerMenuFromHere(millis());
    uiActionSwallowAll(in);
    return true;
  }
  return false;
}

static bool handleBootNamePetFixup(InputState& in)
{
  // BOOT FIXUP:
  // Only apply this when we are truly in a bad boot resume.
  // DO NOT run this during a legitimate new-pet flow (egg->hatch->name).
  if (!s_bootNamePetFixApplied && g_app.uiState == UIState::NAME_PET)
  {
    s_bootNamePetFixApplied = true;

    // If we're actively in the new-pet flow, NAME_PET is valid.
    if (g_app.newPetFlowActive)
    {
      uiActionSwallowAll(in);
      return true;
    }

    // Otherwise, treat NAME_PET as suspicious only if we are not text-capturing.
    if (!g_textCaptureMode)
    {
      inputSetTextCapture(false);
      g_textCaptureMode = false;

      g_app.uiState    = UIState::CHOOSE_PET;
      g_app.currentTab = Tab::TAB_PET;

      g_choosePetBlockHatchUntilRelease = true;

      requestUIRedraw();
      invalidateBackgroundCache();
      requestUIRedraw();

      uiActionSwallowAll(in);
      return true;
    }
  }

  return false;
}

static bool handleMenuSuppression(InputState& in)
{
  // Suppress menu/esc edges for a brief window after leaving overlays.
  if (menuSuppressedNow() && (in.menuOnce || in.escOnce))
  {
    uiActionSwallowAll(in);
    return true;
  }
  return false;
}

static bool handleOpenSettings(InputState& in)
{
  // Classic behavior: ESC (or hotSettings) opens Settings from allowed states
  if (canOpenSettingsFrom(g_app.uiState) && (in.escOnce || in.hotSettings))
  {
    openSettingsFromHere(in);
    return true;
  }
  return false;
}

static bool handleSleepingGate(InputState& in)
{
  // HARD INPUT GATE WHILE SLEEPING
  //  - ENTER wakes (select/encoderPress)
  //  - ESC opens Settings WITHOUT waking
  //  - EVERYTHING ELSE is swallowed
  if (g_app.uiState == UIState::PET_SLEEPING)
  {
    static bool s_prevSleepSelectHeld = false;
    const bool enterEdge = (in.selectHeld && !s_prevSleepSelectHeld);
    s_prevSleepSelectHeld = in.selectHeld;

    const bool wakePressed = (enterEdge || in.encoderPressOnce || in.selectOnce);

    if (in.escOnce && !wakePressed)
    {
      openSettingsFromHere(in);
      return true;
    }

    if (!wakePressed)
    {
      // swallow everything except the wake action
      uiActionDrainKb(in);
      in.clearEdges();
      return true;
    }
  }

  return false;
}

static bool handleFactoryResetOverlay(InputState& in)
{
  if (!g_factoryReset.confirmActive)
    return false;

  // Confirm dialog is fully handled by flow_factory_reset.
  // IMPORTANT: DO NOT route to uiFactoryResetHandle() (it is a stub).
  // When confirmActive is true, the systemSettingsIndex parameter is unused.
  factoryResetSystemSettingsHook(in, 0);
  return true;
}

// --------------------------------------------------------------
// Global interceptors (ordered)
// Return true if handled (and input is swallowed/mutated accordingly)
// --------------------------------------------------------------
bool uiHandleGlobalInterceptors(InputState& in)
{
  // Priority order matters.
  if (handlePowerMenuOverlay(in)) return true;
  if (handleFactoryResetOverlay(in)) return true;
  if (handlePowerMenuOpen(in)) return true;
  if (handleBootNamePetFixup(in)) return true;
  if (handleMenuSuppression(in)) return true;
  if (handleOpenSettings(in)) return true;
  if (handleSleepingGate(in)) return true;

  return false;
}