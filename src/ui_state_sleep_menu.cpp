#include "ui_state_sleep_menu.h"

#include <Arduino.h>

#include "app_state.h"
#include "graphics.h"
#include "input.h"
#include "pet.h"
#include "save_manager.h"
#include "sleep_state.h"
#include "ui_input_common.h"
#include "ui_sleep_menu.h"
#include "ui_suppress.h"

namespace {

struct SleepChoiceDef {
  bool untilAwakened;
  bool untilRested;
  uint32_t durationMs; // 0 means "no fixed duration"
};

// Order must match the menu labels / expected UI ordering
static const SleepChoiceDef kSleepChoices[] = {
  { true,  false, 0 },                         // 0: Until Awakened
  { false, true,  0 },                         // 1: Until Rested
  { false, false, 4UL * 60UL * 60UL * 1000UL }, // 2: 4 hours
  { false, false, 8UL * 60UL * 60UL * 1000UL }, // 3: 8 hours
};

static inline int sleepChoiceCount() {
  return (int)(sizeof(kSleepChoices) / sizeof(kSleepChoices[0]));
}

static inline void swallowInputForTransition(InputState& in, uint32_t menuSuppressMs)
{
  // Prevent ESC/MENU “residue” from immediately re-triggering another overlay.
  if (menuSuppressMs > 0) uiSuppressMenuForMs(menuSuppressMs);

  // Strong hygiene: wipe edges + any queued typing + latch residue.
  in.clearEdges();
  inputForceClear();
  clearInputLatch();
}

static void goBackToPetScreen(InputState& in)
{
  g_app.uiState    = UIState::PET_SCREEN;
  g_app.currentTab = Tab::TAB_PET;

  requestUIRedraw();
  invalidateBackgroundCache();

  // Short suppression is enough here; we’re not dealing with sleep/wake.
  swallowInputForTransition(in, 250);
}

static void enterSleepCommon(InputState& in)
{
  pet.isSleeping   = true;
  g_app.isSleeping = true;

  g_app.uiState    = UIState::PET_SLEEPING;
  g_app.currentTab = Tab::TAB_PET;

  // prevent the ENTER that starts sleep from waking instantly
  uiSuppressSleepWakeForMs(400);

  requestUIRedraw();
  invalidateBackgroundCache();

  swallowInputForTransition(in, 250);

  g_app.sleepTargetEnergy = 0;
  saveManagerMarkDirty();

  sleepBgKickNow();
}

static void applySleepChoice(InputState& in, int idx)
{
  if (idx < 0 || idx >= sleepChoiceCount()) return;

  const SleepChoiceDef& c = kSleepChoices[idx];

  g_app.sleepUntilAwakened = c.untilAwakened;
  g_app.sleepUntilRested   = c.untilRested;
  g_app.sleepStartTime     = millis();
  g_app.sleepDurationMs    = c.durationMs;

  enterSleepCommon(in);
}

} // namespace

void uiSleepMenuHandle(InputState& in)
{
  const int totalItems = sleepChoiceCount();

  // Navigation (unified)
  const int move = uiNavMove(in);
  if (move != 0) {
    g_app.sleepMenuIndex += move;
    uiWrapIndex(g_app.sleepMenuIndex, totalItems);
    requestUIRedraw();
    return;
  }

  // Back (unified)
  if (uiBackPressed(in)) {
    goBackToPetScreen(in);
    return;
  }

  // Select (unified)
  if (!uiSelectPressed(in)) return;

  applySleepChoice(in, g_app.sleepMenuIndex);
}