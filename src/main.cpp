#include <Arduino.h>
#include "app_setup.h"
#include "app_loop.h"
#include "state_manager.h"
#include "boot_state.h"

void setup() {
    appSetup();                              // real hardware init: M5, display, SD, etc.
    state_manager.setStateOwned(new BootState());}

void loop() {
    appMainLoopTick(); // real game loop: input, rendering, pet tick, wifi, etc.
    state_manager.update(); // your state machine tick
}