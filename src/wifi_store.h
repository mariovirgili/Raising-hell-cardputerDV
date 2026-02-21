#pragma once
#include <Arduino.h>

bool wifiStoreLoad(String& ssid, String& pass);     // true if ssid exists
void wifiStoreSave(const String& ssid, const String& pass);
void wifiStoreClear();
bool wifiStoreHasCreds();
