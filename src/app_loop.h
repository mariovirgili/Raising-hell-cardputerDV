#pragma once
#include <stdint.h>

struct InputState;

// Runs one full frame/tick of the application loop.
// This contains what currently lives inside loop().
void appMainLoopTick();
