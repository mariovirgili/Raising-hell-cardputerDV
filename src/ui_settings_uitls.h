#pragma once

// Keep helpers header-only so we don’t create link dependencies.
#include "sound.h"
#include "sdcard.h"
#include "save_manager.h"
#include "ui_runtime.h"
#include "input.h"

// Standard “commit” sequence used by settings actions
static inline void settingsCommitAndBeep(InputState& input) {
  saveSettingsToSD();
  saveManagerMarkDirty();
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}

// Standard “UI feedback” sequence (no save)
static inline void settingsRedrawBeep(InputState& input) {
  requestUIRedraw();
  playBeep();
  clearInputLatch();
}