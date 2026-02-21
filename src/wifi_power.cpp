#include "wifi_power.h"

#include <WiFi.h>
#include <Preferences.h>
#include "wifi_power.h"
#include "wifi_store.h"

void applyWifiPower(bool enable)
{
  if (!enable) {
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    return;
  }

  // enable
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  String ssid, pass;
  if (wifiStoreLoad(ssid, pass)) {
    WiFi.begin(ssid.c_str(), pass.c_str());
  } else {
    // No creds stored => do NOT connect
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true, true);
  }
}

void wifiResetSettings()
{
  // Clear stored creds (matches your console code namespace/keys)
  Preferences prefs;
  prefs.begin("rh_wifi", false);
  prefs.putString("ssid", "");
  prefs.putString("pass", "");
  prefs.end();

  // Disconnect from current network
  WiFi.disconnect(true, true);
  delay(50);
}
