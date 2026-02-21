#pragma once
#include <stdint.h>

// Central home for time snapshot state used by UI + gameplay.
// Keep it tiny and explicit.

extern int currentHour;              // 0-23
extern int currentMinute;            // 0-59
extern uint32_t lastMinuteUpdate;    // millis() timestamp of last update

void initTimeNTP();
void updateTime();
void setTime(int hour, int minute);
void loadTimeFromEEPROM();
