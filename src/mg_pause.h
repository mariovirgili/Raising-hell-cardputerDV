#pragma once
#include <stdint.h>
#include "mg_pause.h"

struct InputState;

void     mgPauseReset();
void     mgPauseForceOffNoStick();
void     mgPauseUpdateClocks(uint32_t nowMs);
bool     mgPauseIsPaused();

uint32_t mgPauseAccumMs();
uint32_t mgPauseStartMs();
bool     mgPauseJustResumedConsume();

uint8_t  mgPauseHandle(const InputState& in);

// drawing
void     mgDrawPauseOverlay();