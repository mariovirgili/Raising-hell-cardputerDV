#include "timezone.h"
#include <time.h>
#include <stdlib.h>

static const char* kTzNames[] = {
  "UTC",
  "US/Eastern",
  "US/Central",
  "US/Mountain",
  "US/Pacific",
  "US/Alaska",
  "US/Hawaii"
};

// POSIX TZ strings with DST rules
static const char* kTzPosix[] = {
  "UTC0",
  "EST5EDT,M3.2.0/2,M11.1.0/2",
  "CST6CDT,M3.2.0/2,M11.1.0/2",
  "MST7MDT,M3.2.0/2,M11.1.0/2",
  "PST8PDT,M3.2.0/2,M11.1.0/2",
  "AKST9AKDT,M3.2.0/2,M11.1.0/2",
  "HST10"
};

static const uint8_t kTzCount =
  sizeof(kTzNames) / sizeof(kTzNames[0]);

uint8_t tzCount() {
  return kTzCount;
}

const char* tzName(uint8_t idx) {
  if (idx >= kTzCount) idx = 0;
  return kTzNames[idx];
}

void applyTimezoneIndex(uint8_t idx) {
  if (idx >= kTzCount) idx = 0;
  setenv("TZ", kTzPosix[idx], 1);
  tzset();
}

#include <Preferences.h>

static constexpr const char* NVS_NS  = "rh_sys";
static constexpr const char* NVS_KEY = "tzIndex";

bool loadTzIndexFromNVS(uint8_t* outIdx) {
  if (!outIdx) return false;

  Preferences p;
  if (!p.begin(NVS_NS, true)) return false;

  bool has = p.isKey(NVS_KEY);
  uint8_t v = p.getUChar(NVS_KEY, 0);
  p.end();

  if (!has) return false;
  if (v >= tzCount()) return false;

  *outIdx = v;
  return true;
}

void saveTzIndexToNVS(uint8_t idx) {
  if (idx >= tzCount()) idx = 0;

  Preferences p;
  if (!p.begin(NVS_NS, false)) return;
  p.putUChar(NVS_KEY, idx);
  p.end();
}
