#include "ui_feed_menu.h"

#include "input.h"
#include "ui_menu_state.h"       // feedMenuIndex
#include "feed_menu_actions.h"   // feedUseSelected()

namespace {

struct MenuItem {
  const char* label;
  void (*onSelect)(InputState&);
};

static void actUseSelected(InputState& in)
{
  (void)in;
  feedUseSelected();
}

static const MenuItem kItems[] = {
  // Keep labels consistent with whatever draw function you currently have.
  // If you rename them, only do it here and your renderer will follow.
  {"Just a Snack",   actUseSelected},
  {"Until Full",   actUseSelected},
};

} // namespace

int uiFeedMenuCount()
{
  return (int)(sizeof(kItems) / sizeof(kItems[0]));
}

const char* uiFeedMenuLabel(int idx)
{
  if (idx < 0 || idx >= uiFeedMenuCount()) return "";
  return kItems[idx].label;
}

bool uiFeedMenuActivate(int idx, InputState& in)
{
  if (idx < 0 || idx >= uiFeedMenuCount()) return false;
  if (!kItems[idx].onSelect) return false;

  // Ensure the legacy action reads the correct selection.
  feedMenuIndex = idx;

  kItems[idx].onSelect(in);
  return true;
}