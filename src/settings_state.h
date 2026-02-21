// settings_state.h
#pragma once
#include "ui_defs.h"
#include "ui_state_utils.h" 
// keep any other settings_state.h declarations below...

#include <stdint.h>
#include <time.h>

#include "ui_defs.h"
#include "pet.h"  // for PET_NAME_MAX

#include "user_toggles_state.h"
#include "settings_nav_state.h"
#include "runtime_flags_state.h"

#include "time_editor_state.h" // canonical g_setTime* declarations

// Settings controls
extern uint8_t autoScreenTimeoutSel;

extern bool petDeathEnabled;
