#include "ui_mg_pause_menu.h"

#include "input.h"
#include "mg_pause_core.h"   // mgPauseSetPaused()
#include "mini_games.h"      // miniGameExitToReturnUi()

namespace
{
  struct MenuItem
  {
    const char* label;
    void (*onSelect)(InputState&);
  };

  static void actContinue(InputState& in)
  {
    (void)in;
    mgPauseSetPaused(false);
  }

  static void actExit(InputState& in)
  {
    (void)in;

    // Ensure we are not paused anymore, then exit back to the return UI.
    mgPauseSetPaused(false);
    miniGameExitToReturnUi(true);
  }

  static const MenuItem kItems[] =
  {
    { "Continue", actContinue },
    { "Exit",     actExit     },
  };
} // namespace

int uiMgPauseMenuCount()
{
  return (int)(sizeof(kItems) / sizeof(kItems[0]));
}

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