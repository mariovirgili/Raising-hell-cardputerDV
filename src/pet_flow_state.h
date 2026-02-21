#pragma once
#include <Arduino.h>
#include <stdint.h>

struct HatchFlowState {
  bool     active = false;
  uint32_t startMs = 0;

  bool     showingMsg = false;
  uint32_t msgStartMs = 0;

  uint8_t  frame = 0;

  bool     flashWhite = false;
  uint32_t flashStartMs = 0;
};

struct EvoFlowState {
  bool     active = false;

  bool     showingMsg = false;
  uint32_t msgStartMs = 0;

  uint8_t  fromStage = 0;
  uint8_t  toStage   = 0;

  uint8_t  phase = 0;            // 0=pre, 1=flash, 2=post, 3=done
  uint32_t phaseStartMs = 0;

  bool     flashWhite = false;
};

struct PetFlowState {
  HatchFlowState hatch;
  EvoFlowState   evo;
};
