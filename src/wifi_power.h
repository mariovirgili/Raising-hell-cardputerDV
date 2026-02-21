#pragma once
#include <Arduino.h>

void applyWifiPower(bool enable);

// NEW:
void wifiResetSettings();   // clears saved ssid/pass + disconnects
