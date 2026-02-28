#include "ui_state_pet_sleeping.h"

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "led_status.h" // isPetSleepingNow()
#include "pet.h"
#include "ui_actions.h"
#include "ui_runtime.h"

// Pet Sleep Screen behavior:
// - If the pet stops sleeping (timer/rest complete), return to PET_SCREEN.
// - MENU/ESC opens Settings without waking the pet.
// - ENTER/G (selectOnce/encoderPressOnce) wakes the pet and returns to PET_SCREEN.

void uiPetSleepingHandle(InputState &in)
{
  // If we're not sleeping anymore, bounce back to pet screen.
  if (!isPetSleepingNow())
  {
    uiActionEnterStateClean(UIState::PET_SCREEN, Tab::TAB_PET, true, in, 200);
    invalidateBackgroundCache();
    requestUIRedraw();
    return;
  }

  // Some keyboard firmwares don't always emit a clean selectOnce edge in this state.
  // Add a held->edge fallback so ENTER still wakes reliably.
  static bool s_prevSelectHeld = false;
  const bool selectEdgeFallback = (in.selectHeld && !s_prevSelectHeld);
  s_prevSelectHeld = in.selectHeld;

  // Open settings while sleeping (must NOT wake pet).
  if (in.menuOnce || in.escOnce)
  {
    uiActionEnterStateClean(UIState::SETTINGS, g_app.currentTab, true, in, 150);
    requestUIRedraw();
    return;
  }

  // Wake explicitly on enter/select.
  // (mgSelectOnce is harmless here; it's usually only set in mini-game, but allows
  // alternate firmwares/mappings to still wake.)
  if (in.selectOnce || in.encoderPressOnce || in.mgSelectOnce || selectEdgeFallback)
  {
    // Clear global sleep intent flags.
    g_app.isSleeping = false;
    g_app.sleepUntilAwakened = false;
    g_app.sleepUntilRested = false;
    g_app.sleepingByTimer = false;

    // Clear the pet's sleep flag too.
    pet.isSleeping = false;

    uiActionEnterStateClean(UIState::PET_SCREEN, Tab::TAB_PET, true, in, 200);
    invalidateBackgroundCache();
    requestUIRedraw();
    return;
  }
}