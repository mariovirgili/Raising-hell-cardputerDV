#pragma once
#include <stddef.h>
#include <stdint.h>

extern bool g_debugEnabled;
extern uint32_t g_debugArmMs;

bool dbgCanWrite(size_t needBytes);

void pollDebugPort();
void keyboardDebugTick();
