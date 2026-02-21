#include "wifi_store.h"

#include <Preferences.h>

static const char* NS     = "rh_wifi";
static const char* K_SSID = "ssid";
static const char* K_PASS = "pass";

bool wifiStoreHasCreds() {
  Preferences p;
  if (!p.begin(NS, true)) return false;
  String s = p.getString(K_SSID, "");
  p.end();
  return s.length() > 0;
}

bool wifiStoreLoad(String& ssid, String& pass) {
  Preferences p;
  if (!p.begin(NS, true)) return false;
  ssid = p.getString(K_SSID, "");
  pass = p.getString(K_PASS, "");
  p.end();
  return ssid.length() > 0;
}

void wifiStoreSave(const String& ssid, const String& pass) {
  Preferences p;
  if (!p.begin(NS, false)) return;
  p.putString(K_SSID, ssid);
  p.putString(K_PASS, pass);
  p.end();
}

void wifiStoreClear() {
  Preferences p;
  if (!p.begin(NS, false)) return;
  p.remove(K_SSID);
  p.remove(K_PASS);
  p.end();
}
