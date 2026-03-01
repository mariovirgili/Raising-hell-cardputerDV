#include "ui_mg_pause_menu.h"
#include "mini_games.h"
#include "input.h"
#include "mg_pause.h"   // <-- declares mgPauseSetPaused, mgPauseChoice, etc.
#include "mini_game_return_ui.h" // whatever you use to resume/exit
// If your project uses different functions, we’ll map them 1:1.

namespace {

struct MenuItem { const char* label; void (*onSelect)(InputState&); };

static void actContinue(InputState& in)
{
  (void)in;
  mgPauseSetPaused(false);   // <-- replace with your existing resume function
}

static void actExit(InputState& in)
{
  (void)in;
  miniGameExitToReturnUi(true);     // <-- replace with your existing exit function
}

static const MenuItem kItems[] = {
  {"Continue", actContinue},
  {"Exit",     actExit},
};

} // namespace

int uiMgPauseMenuCount() { return (int)(sizeof(kItems)/sizeof(kItems[0])); }
const char* uiMgPauseMenuLabel(int idx)
{
  if (idx < 0 || idx >= uiMgPauseMenuCount()) return "";
  return kItems[idx].label;
}
bool uiMgPauseMenuActivate(int idx, InputState& in)
{
  if (idx < 0 || idx >= uiMgPauseMenuCount()) return false;
  if (!kItems[idx].onSelect) return false;
  kItems[idx].onSelect(in);
  return true;
}