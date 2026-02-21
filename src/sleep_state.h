// sleep_state.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

void sleepMiniStatsHeartbeat(uint32_t now);

// Sleep globals (used across pet/sleep/ui)
extern bool isSleeping;
extern bool sleepingByTimer;

// Sleep modes (flags)
extern bool sleepUntilRested;
extern bool sleepUntilAwakened;

// Sleep session details
extern uint8_t  sleepTargetEnergy;
extern uint32_t sleepStartTime;
extern uint32_t sleepDurationMs;
