#include "app_loop_helpers.h"

// Arduino / system
#include <Arduino.h>
#include <type_traits>
#include "M5Cardputer.h"

// Project
#include "app_state.h"
#include "auto_screen.h"
#include "debug_state.h"
#include "display.h"
#include "display_dims_state.h"
#include "led_status.h"
#include "pet.h"
#include "sdcard.h"
#include "sleep_state.h"
#include "time_state.h"
#include "ui_runtime.h"


void tickTimeDisplay() {
  // Keep all time validity/anchor gating inside the time module.
  updateTime();
}
