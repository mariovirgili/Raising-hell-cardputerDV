#pragma once
#include <stdint.h>

struct InputState;

// Core pause API
void     mgPauseReset();
void     mgPauseForceOffNoStick();
void     mgPauseUpdateClocks(uint32_t nowMs);
bool     mgPauseIsPaused();

uint32_t mgPauseAccumMs();
uint32_t mgPauseStartMs();
bool     mgPauseJustResumedConsume();

uint8_t  mgPauseHandle(const InputState& in);

// Drawing
void     mgDrawPauseOverlay();

// Menu helpers (mgPauseSetPaused / choice helpers)
#include "mg_pause_menu.h"