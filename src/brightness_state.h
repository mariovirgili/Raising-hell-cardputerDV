#pragma once
#include <cstdint>

extern uint8_t brightnessLevel;
extern const uint16_t brightnessValues[3];

void applyBrightnessLevel(int level);
