// system_status_state.cpp
#include <Arduino.h>
#include "system_status_state.h"

int      batteryPercent = 0;
bool     usbPowered = false;
uint32_t bootTime = 0;
