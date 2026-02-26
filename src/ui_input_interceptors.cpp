#include "ui_input_interceptors.h"

#include <Arduino.h> // millis()

#include "app_state.h"
#include "input.h"

#include "console.h"
#include "factory_reset_state.h"
#include "flow_factory_reset.h"
#include "flow_power_menu.h"
#include "graphics.h"
#include "new_pet_flow_state.h"
#include "settings_flow_state.h"
#include "settings_nav_state.h"
#include "ui_actions.h"
#include "ui_defs.h"
#include "ui_input_common.h"
#include "ui_runtime.h"
#include "ui_suppress.h"

static bool handleEscGlobal(InputState &in);

// Keep the boot fix local to this module.
static bool s_bootNamePetFixApplied = false;

// Menu/ESC suppression is centralized in ui_suppress.*
static inline bool menuSuppressedNow() { return uiIsMenuSuppressed(); }

// Local “swallow everything” helper (removes dependency on uiActionSwallowAll).
static inline void swallowAll(InputState& in)
{
  uiActionDrainKb(in);
  uiActionSwallowEdges(in);
  clearInputLatch();
}

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

static void openSettingsFromHere(InputState &in)
{
  g_settingsFlow.settingsReturnState = g_app.uiState;
  g_settingsFlow.settingsReturnTab = g_app.currentTab;
  g_settingsFlow.settingsReturnValid = true;

  resetSettingsNav(true);
  g_settingsFlow.settingsPage = SettingsPage::TOP;

  uiActionEnterState(UIState::SETTINGS, g_app.currentTab, true);

  // Prevent the same ESC/MENU press from being re-consumed immediately.
  uiSuppressMenuForMs(250);
  swallowAll(in);
}

// --------------------------------------------------------------
// Interceptor handlers (small, ordered, behavior-preserving)
// --------------------------------------------------------------
static bool handlePowerMenuOverlay(InputState &in)
{
  if (g_app.uiState == UIState::POWER_MENU)
  {
    uiPowerMenuHandle(in);
    return true;
  }
  return false;
}

static bool handlePowerMenuOpen(InputState &in)
{
  // Open power menu ONLY via GO long-hold (do not tie to menuOnce/escOnce).
  if (!g_textCaptureMode && in.goLongHold)
  {
    openPowerMenuFromHere(millis());
    swallowAll(in);
    return true;
  }
  return false;
}

static bool handleBootNamePetFixup(InputState &in)
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
      swallowAll(in);
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

      swallowAll(in);
      return true;
    }
  }

  return false;
}

static bool handleMenuSuppression(InputState &in)
{
  // Suppress menu edges for a brief window after leaving overlays.
  // IMPORTANT: do NOT swallow ESC here, or it takes extra presses to exit Settings.
  if (menuSuppressedNow() && (in.menuOnce || in.hotSettings))
  {
    swallowAll(in);
    return true;
  }
  return false;
}

static bool handleOpenSettings(InputState &in)
{
  // Classic behavior: ESC (or hotSettings) opens Settings from allowed states
  if (canOpenSettingsFrom(g_app.uiState) && (in.escOnce || in.hotSettings))
  {
    openSettingsFromHere(in);
    return true;
  }
  return false;
}

static bool handleSleepingGate(InputState &in)
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

static bool handleFactoryResetOverlay(InputState &in)
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
bool uiHandleGlobalInterceptors(InputState &in)
{
  // Let the console own ESC/MENU entirely.
  if (g_app.uiState == UIState::CONSOLE)
    return false;

  if (handleEscGlobal(in))
    return true;
  if (handlePowerMenuOverlay(in))
    return true;
  if (handleFactoryResetOverlay(in))
    return true;
  if (handlePowerMenuOpen(in))
    return true;
  if (handleBootNamePetFixup(in))
    return true;
  if (handleMenuSuppression(in))
    return true;
  if (handleOpenSettings(in))
    return true;
  if (handleSleepingGate(in))
    return true;

  return false;
}

static bool handleEscGlobal(InputState &in)
{
  if (!in.escOnce)
    return false;

  // Let states that already own ESC handle it.
  // (Avoid swallowing ESC and breaking “dismiss console/power menu/settings”.)
  switch (g_app.uiState)
  {
  case UIState::CONSOLE:
  case UIState::POWER_MENU:
  case UIState::SETTINGS:
  case UIState::MINI_GAME:
  case UIState::MG_PAUSE:
    return false;

  default:
    break;
  }

  // Default behavior: ESC opens Settings from anywhere else.
  openSettingsFromHere(in);
  in.escOnce = false;
  return true;
}