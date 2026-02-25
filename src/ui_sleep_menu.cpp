#include "ui_sleep_menu.h"

#include "app_state.h"
#include "input.h"
#include "sound.h"
#include "ui_invalidate.h"

// If you already have specific functions for sleep/wake/cancel,
// include the right headers and call them here.
// Keep this file as the single place that maps menu index -> action.

namespace {

struct SleepMenuItem {
  const char* label;
  void (*onSelect)();
};

static void actSleepNow() {
  // TODO: call your existing "enter sleep" action here
  // Example (replace with real one):
  // enterSleepMode();
  playBeep();
  requestUIRedraw();
}

static void actWakeUp() {
  // TODO: call your existing "wake" action here
  playBeep();
  requestUIRedraw();
}

static void actCancel() {
  // Back to wherever you came from; replace with your existing behavior.
  g_app.uiState = UIState::PET_SCREEN;
  playBeep();
  requestUIRedraw();
}

static const SleepMenuItem kItems[] = {
  {"SLEEP",  actSleepNow},
  {"WAKE",   actWakeUp},
  {"CANCEL", actCancel},
};

} // namespace

int uiSleepMenuCount() {
  return (int)(sizeof(kItems) / sizeof(kItems[0]));
}

const char* uiSleepMenuLabel(int idx) {
  if (idx < 0 || idx >= uiSleepMenuCount()) return "";
  return kItems[idx].label;
}

void uiSleepMenuActivate(int idx) {
  if (idx < 0 || idx >= uiSleepMenuCount()) return;
  if (kItems[idx].onSelect) kItems[idx].onSelect();
}