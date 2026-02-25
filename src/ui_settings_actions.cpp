#include "ui_settings_actions.h"

#include <Arduino.h>

#include "debug.h"
#include "sdcard.h"
#include "sound.h"
#include "time_state.h"
#include "time_persist.h"
#include "timezone.h"
#include "ui_runtime.h"
#include "input.h"   
#include "save_manager.h"

void settingsCycleTimeZone(int dir) {
  const int count = (int)tzCount();
  if (count <= 0) return;

  int next = (int)tzIndex + dir;
  while (next < 0) next += count;
  next %= count;

  tzIndex = (uint8_t)next;

  applyTimezoneIndex(tzIndex);
  updateTime();

  saveSettingsToSD();
  saveTzIndexToNVS(tzIndex);

  uint8_t rb = 255;
  if (loadTzIndexFromNVS(&rb)) {
    DBG_ON("[TZ] saved idx=%u readback=%u\n", (unsigned)tzIndex, (unsigned)rb);
  } else {
    DBG_ON("[TZ] saved idx=%u readback FAILED\n", (unsigned)tzIndex);
  }

  requestFullUIRedraw();
  playBeep();
  clearInputLatch();
}