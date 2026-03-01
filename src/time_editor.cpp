#include "time_editor.h"

#include "time_state.h"

// 0 = hour, 1 = minute
static int s_field = 0;
static int s_hour = 0;
static int s_minute = 0;

static inline int wrap(int v, int lo, int hi)
{
  const int span = (hi - lo) + 1;
  while (v < lo) v += span;
  while (v > hi) v -= span;
  return v;
}

void timeEditorBegin(bool /*force24h*/)
{
  s_field = 0;
  s_hour = wrap((int)currentHour, 0, 23);
  s_minute = wrap((int)currentMinute, 0, 59);
}

void timeEditorPrevField()
{
  s_field = (s_field == 0) ? 1 : 0;
}

void timeEditorNextField()
{
  s_field = (s_field == 0) ? 1 : 0;
}

void timeEditorInc()
{
  if (s_field == 0) s_hour = wrap(s_hour + 1, 0, 23);
  else              s_minute = wrap(s_minute + 1, 0, 59);
}

void timeEditorDec()
{
  if (s_field == 0) s_hour = wrap(s_hour - 1, 0, 23);
  else              s_minute = wrap(s_minute - 1, 0, 59);
}

void timeStateCommitFromEditor()
{
  currentHour = (uint8_t)wrap(s_hour, 0, 23);
  currentMinute = (uint8_t)wrap(s_minute, 0, 59);
  // Note: this project keeps time at minute resolution in time_state.*; there is no
  // separate seconds field to reset here.
}

int timeEditorHour() { return s_hour; }
int timeEditorMinute() { return s_minute; }
int timeEditorField() { return s_field; }
