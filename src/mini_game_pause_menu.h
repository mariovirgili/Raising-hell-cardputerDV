#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "input.h"

// Return codes for miniGamePauseHandle()
static constexpr uint8_t MINI_GAME_PAUSE_NONE    = 0;
static constexpr uint8_t MINI_GAME_PAUSE_CONSUME = 1; // consumed input (pause toggled / selection moved, etc)
static constexpr uint8_t MINI_GAME_PAUSE_EXIT    = 2; // user chose exit mini-game

bool     miniGamePauseIsPaused();
void     miniGamePauseSetPaused(bool paused);

uint8_t  miniGamePauseHandle(const InputState& input);

void     miniGamePauseUpdateClocks(uint32_t now);
void     miniGamePauseResetClock();
void     miniGamePauseReset();
void     miniGamePauseForceOffNoStick();

uint32_t miniGamePauseAccumMs();
uint32_t miniGamePauseStartMs();
bool     miniGamePauseJustResumedConsume();

void     miniGameDrawPauseOverlay();