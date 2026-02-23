#include <Arduino.h>
#include "app_setup.h"
#include "app_loop.h"
#include "state_manager.h"
#include "boot_state.h"

// Single global definition of state_manager
StateManager state_manager;

void setup() {
    appSetup();                              // real hardware init: M5, display, SD, etc.
    state_manager.setState(new BootState()); // your state machine boot
}

void loop() {
    appMainLoopTick(); // real game loop: input, rendering, pet tick, wifi, etc.
    state_manager.update(); // your state machine tick
}