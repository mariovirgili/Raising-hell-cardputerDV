// time_editor_state.cpp
#include <time.h>
#include "time_editor_state.h"
#include "ui_defs.h"   // UIState, Tab
#include "settings_state.h"

bool     g_setTimeActive = false;
tm       g_setTimeTm = {};
uint8_t  g_setTimeField = 0;
bool     g_setTimeForceNoCancel = false;

bool     g_setTimeReturnValid = false;

// These are stored as raw u8 in the header, so define as u8.
uint8_t  g_setTimeReturnState = 0;
uint8_t  g_setTimeReturnTab   = 0;
