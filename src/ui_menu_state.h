// ui_menu_state.h
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "ui_runtime.h"

// Menu indices / UI selection state.
//
// IMPORTANT:
// - shopIndex/feedMenuIndex/powerMenuIndex/lastRenderTimeMs are aliases into g_app
//   (single source of truth).
// - sleepMenuIndex/playMenuIndex/selectedMenu remain standalone legacy globals
//   until you migrate them too.

extern int&      shopIndex;
extern int&      feedMenuIndex;
extern int&      powerMenuIndex;
extern uint32_t& lastRenderTimeMs;

// Also aliases into g_app now (single source of truth)
extern int& sleepMenuIndex;
extern int& playMenuIndex;
extern int& selectedMenu;
