#include "ui_sleep_menu.h"

#include <Arduino.h>

#include "input.h"
#include "ui_sleep_menu_actions.h"

namespace {

struct SleepMenuItem {
  const char* label;
  void (*onSelect)(InputState&, uint32_t nowMs);
};

static void actUntilAwakened(InputState&, uint32_t nowMs)
{
  uiSleepMenuSetUntilAwakened(nowMs);
  uiSleepMenuEnterSleep(nowMs);
}

static void actUntilRested(InputState&, uint32_t nowMs)
{
  uiSleepMenuSetUntilRested(nowMs);
  uiSleepMenuEnterSleep(nowMs);
}

static void act4Hours(InputState&, uint32_t nowMs)
{
  uiSleepMenuSetForHours(nowMs, 4);
  uiSleepMenuEnterSleep(nowMs);
}

static void act8Hours(InputState&, uint32_t nowMs)
{
  uiSleepMenuSetForHours(nowMs, 8);
  uiSleepMenuEnterSleep(nowMs);
}

static const SleepMenuItem kItems[] = {
  {"Until Awakened", actUntilAwakened},
  {"Until Rested",   actUntilRested},
  {"4 hours",        act4Hours},
  {"8 hours",        act8Hours},
};

} // namespace

int uiSleepMenuCount()
{
  return (int)(sizeof(kItems) / sizeof(kItems[0]));
}

const char* uiSleepMenuLabel(int idx)
{
  if (idx < 0 || idx >= uiSleepMenuCount()) return "";
  return kItems[idx].label;
}

bool uiSleepMenuActivate(int idx, InputState& in)
{
  if (idx < 0 || idx >= uiSleepMenuCount()) return false;
  if (!kItems[idx].onSelect) return false;

  const uint32_t nowMs = millis();
  kItems[idx].onSelect(in, nowMs);
  return true;
}