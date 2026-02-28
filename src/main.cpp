#include <Arduino.h>
#include "app_setup.h"
#include "state_manager.h"
#include "legacy_app_state.h"
#include <stdint.h>

static LegacyAppState g_legacy;

void setup()
{
  appSetup();
  state_manager.setState(&g_legacy);
}

void loop()
{
  state_manager.update();
  delay(1); // keep this tiny yield; not temp, prevents RTOS/WDT issues
}