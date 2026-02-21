// wifi_setup_state.cpp
#include "wifi_setup_state.h"

WifiSetupState g_wifi;

bool g_wifiSetupFromBootWizard = false;

// Legacy reference aliases
int& wifiSettingsIndex = g_wifi.wifiSettingsIndex;
uint8_t& wifiSetupStage = g_wifi.setupStage;
char (&wifiSetupSsid)[33] = g_wifi.ssid;
char (&wifiSetupPass)[65] = g_wifi.pass;
char (&wifiSetupBuf)[65]  = g_wifi.buf;
