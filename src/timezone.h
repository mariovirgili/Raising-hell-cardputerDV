#pragma once
#include <stdint.h>

extern int tzIndex;

uint8_t tzCount();
const char* tzName(uint8_t idx);
void applyTimezoneIndex(uint8_t idx);

// NEW: persist tzIndex to NVS
bool loadTzIndexFromNVS(uint8_t* outIdx);   // returns true if present+valid
void saveTzIndexToNVS(uint8_t idx);
