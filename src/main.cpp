#include <Arduino.h>
#include "app_setup.h"
#include "state_manager.h"
#include "legacy_app_state.h"
#include <stdint.h>
#include "app_loop.h"

void appMainLoopTick();
void appServicesTick(uint32_t nowMs);
static LegacyAppState g_legacy;

void setup() {
    appSetup();
    state_manager.setState(&g_legacy);
}

void loop() {
    const uint32_t now = millis();
    appServicesTick(now); 
    state_manager.update();
  }