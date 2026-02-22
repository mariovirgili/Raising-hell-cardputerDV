#include <Arduino.h>
#include "app_setup.h"
#include "app_loop.h"

void setup() {
    appSetup();
}

void loop() {
    appMainLoopTick();
}
