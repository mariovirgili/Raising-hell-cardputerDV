#include "ui_death_menu.h"

#include "app_state.h"
#include "graphics.h"     // invalidateBackgroundCache()
#include "input.h"
#include "mini_games.h"   // startResurrectionRun(), currentMiniGame, MiniGame::RESURRECTION
#include "sound.h"
#include "ui_runtime.h"   // requestUIRedraw()

extern int deathMenuIndex;

namespace {

struct MenuItem {
  const char* label;
  void (*onSelect)(InputState&);
};

static void actResurrect(InputState& in)
{
  (void)in;

  g_app.inMiniGame = true;
  g_app.gameOver   = false;
  g_app.uiState    = UIState::MINI_GAME;
  requestUIRedraw();

  invalidateBackgroundCache();

  startResurrectionRun();
  currentMiniGame = MiniGame::RESURRECTION;

  inputForceClear();
  clearInputLatch();
}

static void actBury(InputState& in)
{
  (void)in;

  soundResetDeathDirgeLatch();
  soundFuneralDirge();

  g_app.uiState = UIState::BURIAL_SCREEN;
  requestUIRedraw();

  invalidateBackgroundCache();

  inputForceClear();
  clearInputLatch();
}

static const MenuItem kItems[] = {
  {"RESURRECT", actResurrect},
  {"BURY",      actBury},
};

} // namespace

int uiDeathMenuCount()
{
  return (int)(sizeof(kItems) / sizeof(kItems[0]));
}

const char* uiDeathMenuLabel(int idx)
{
  if (idx < 0 || idx >= uiDeathMenuCount()) return "";
  return kItems[idx].label;
}

bool uiDeathMenuActivate(int idx, InputState& in)
{
  if (idx < 0 || idx >= uiDeathMenuCount()) return false;
  if (!kItems[idx].onSelect) return false;

  // Keep legacy index source-of-truth consistent.
  deathMenuIndex = idx;

  // Match old handler behavior: clear latches before acting.
  clearInputLatch();
  inputForceClear();

  kItems[idx].onSelect(in);
  return true;
}