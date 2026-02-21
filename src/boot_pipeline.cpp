// -----------------------------------------------------------------------------
// Raising Hell — Cardputer ADV Edition
// Boot pipeline implementation (moved out of .ino / out of headers)
// -----------------------------------------------------------------------------
#include "boot_pipeline.h"

#include <Arduino.h>
#include <SD.h>
#include <EEPROM.h>
#include <cstring>

// Keep includes broad/safe like you’ve been doing
#include "sdcard.h"
#include "display.h"
#include "display_state.h"
#include "ui_runtime.h"
#include "input.h"
#include "save_manager.h"
#include "time_persist.h"
#include "wifi_time.h"
#include "wifi_power.h"
#include "timezone.h"
#include "brightness_state.h"
#include "settings_state.h"
#include "controls_help_state.h"
#include "boot_state.h"
#include "inventory.h"
#include "app_state.h"
#include "debug.h"
#include "time_state.h"
#include "eeprom_addrs.h"
#include "graphics.h"
#include "menu_actions.h"
#include "ui_level_popup.h"
#include "ui_menu_state.h"
#include "app_state.h"
#include "display_dims_state.h"

// -----------------------------------------------------------------------------
// SD Asset Check (all builds)
// -----------------------------------------------------------------------------
static const char* kSdAssetsMarkerPath = "/raising_hell/ASSET_MANIFEST.txt";

bool g_assetsChecked = false;
bool g_assetsMissing = false;


void drawAssetsMissingScreen() {
  displayInit();

  spr.fillScreen(TFT_BLACK);
  spr.setTextDatum(textdatum_t::middle_center);

  const bool sdOk = g_sdReady;

  spr.setTextColor(TFT_RED, TFT_BLACK);
  spr.drawString(sdOk ? "ASSETS MISSING" : "SD CARD NOT DETECTED",
                 SCREEN_W / 2, SCREEN_H / 2 - 36);

  spr.setTextColor(TFT_WHITE, TFT_BLACK);

  if (!sdOk) {
    spr.drawString("Insert SD card with assets", SCREEN_W / 2, SCREEN_H / 2 - 10);
    spr.drawString("then press ENTER to retry", SCREEN_W / 2, SCREEN_H / 2 + 14);
  } else {
    spr.drawString("Assets folder missing or incomplete", SCREEN_W / 2, SCREEN_H / 2 - 10);
    spr.drawString("Press ENTER to retry", SCREEN_W / 2, SCREEN_H / 2 + 14);
  }

  spr.pushSprite(0, 0);
}

// -----------------------------------------------------------------------------
// First Run Flag (factory reset helper)
// -----------------------------------------------------------------------------
static const char* kFirstRunFlagPath = "/raising_hell/first_run.flag";

static bool consumeFirstRunFlagIfPresent() {
  if (!g_sdReady) return false;
  if (!SD.exists(kFirstRunFlagPath)) return false;

  SD.remove(kFirstRunFlagPath);
  return true;
}

static uint32_t g_nextSdTryMs   = 0;
static uint32_t g_nextWifiTryMs = 0;

void bootPipelineKick(uint32_t now, bool usbOpen) {
  // Nudge retry timers so first attempts are slightly delayed
  // (gives USB serial time to open, avoids spamming init early).
  g_nextSdTryMs   = now + (usbOpen ? 800 : 200);
  g_nextWifiTryMs = now + (usbOpen ? 1200 : 500);
}

bool sdAssetsPresent() {
  if (!g_sdReady) return false;
  return SD.exists(kSdAssetsMarkerPath);
}

// -----------------------------------------------------------------------------
// Deferred init state machine (boot pipeline)
// -----------------------------------------------------------------------------
uint32_t g_sdFirstTryMs = 0;
bool     g_sdGaveUp    = false;

static bool     g_postBootInitDone = false;
static bool     g_sdTriedLoad      = false;
static bool     g_ntpSaved         = false;
static bool     g_wifiApplied      = false;
uint8_t         g_sdTryCount       = 0;

// ---- Early TZ/anchor latches (Stage 0) ----
static bool g_tzAppliedEarly     = false;
static bool g_anchorAppliedEarly = false;

// Time display gating (loop also checks these)
bool g_timeAnchorAttempted = false;
bool g_timeAnchorRestored  = false;

// -----------------------------------------------------------------------------
// postBootInitTick() 
// -----------------------------------------------------------------------------
void postBootInitTick() {
  const uint32_t now = millis();

  // ---------------------------------------------------------------------------
  // Stage 0: Apply TZ + anchor early (so localtime_r() is sane ASAP)
  // ---------------------------------------------------------------------------
  if (!g_tzAppliedEarly) {
    uint8_t nvsTz;
    if (loadTzIndexFromNVS(&nvsTz)) {
      tzIndex = nvsTz;
    }
    applyTimezoneIndex(tzIndex);
    g_tzAppliedEarly = true;
  }

  if (!g_anchorAppliedEarly) {
    if (!g_timeAnchorAttempted) {
      g_timeAnchorRestored  = restoreTimeFromAnchor();
      g_timeAnchorAttempted = true;
    }
    updateTime();
    g_anchorAppliedEarly = true;
  }

  // ---------------------------------------------------------------------------
  // Stage 1: SD init retry window (up to 5s)
  // ---------------------------------------------------------------------------
  if (!g_sdReady && !g_sdGaveUp) {
    if (g_sdFirstTryMs == 0) g_sdFirstTryMs = now;

    if ((uint32_t)(now - g_sdFirstTryMs) > 5000) {
      g_sdGaveUp = true;
      ui_setBootSplashActive(false);

      // HARD GATE: SD missing -> treat as assets missing so loop blocks.
      g_assetsChecked = true;
      g_assetsMissing = true;

      requestUIRedraw();
    } else if (now >= g_nextSdTryMs) {
      g_sdTryCount++;
      uint32_t backoff = 250 + (uint32_t)g_sdTryCount * 250;
      if (backoff > 1500) backoff = 1500;
      g_nextSdTryMs = now + backoff;

      requestUIRedraw();

      g_sdReady = initSD();
      DBG_ON("[SD] initSD -> %d\n", (int)g_sdReady);

      if (g_sdReady) {
        g_sdTryCount = 0;

        drawBootSplash();
        invalidateBackgroundCache();
        requestUIRedraw();
        renderUI();

        // Asset pack check — ONLY after SD is ready
        if (!g_assetsChecked) {
          g_assetsChecked = true;
          g_assetsMissing = !sdAssetsPresent();

          if (g_assetsMissing) {
            ui_setBootSplashActive(false);
            requestUIRedraw();
            return; // stop boot pipeline until assets are installed
          }
        }
      }
    }

    // If SD still not ready (and we haven't timed out), stop here.
    if (!g_sdReady && !g_sdGaveUp) return;
  }

  // If SD is up but the asset pack is missing, block further boot work until fixed.
  if (g_assetsMissing) return;

  // ---------------------------------------------------------------------------
  // Stage 2: One-time SD load pipeline (settings + save + time anchor)
  // ---------------------------------------------------------------------------
  if (!g_sdTriedLoad) {
    g_sdTriedLoad = true;

    inputSetTextCapture(false);
    clearInputLatch();
    inputForceClear();

    if (g_sdReady) {
      if (!loadSettingsFromSD()) saveSettingsToSD();
    }

    // FIRST RUN FLAG (from factory reset)
    if (consumeFirstRunFlagIfPresent()) {
      g_controlsHelpSeen = 0;
      saveSettingsToSD();
    }

    // Apply loaded brightness immediately
    if (isScreenOn()) {
      applyBrightnessLevel(brightnessLevel);
    }

    // Ensure TZ is applied again after settings load (tzIndex may have changed)
    applyTimezoneIndex(tzIndex);

    // Try restore time anchor again (safe) after settings/tz is applied
    if (!g_timeAnchorAttempted) {
      g_timeAnchorRestored  = restoreTimeFromAnchor();
      g_timeAnchorAttempted = true;
    }

    bool loadedFromSD = false;
    if (g_sdReady) {
      loadedFromSD = saveManagerLoad();
    } else {
      loadedFromSD = false;
    }

    DBG_ON("[LOAD] saveManagerLoad -> %d\n", (int)loadedFromSD);

    updateTime();
    uiInitLevelPopupTracker();

    // -----------------------------------------------------------------------
    // FIRST BOOT TIME GATE
    // -----------------------------------------------------------------------
    if (!timeIsValid()) {
      const UIState afterOk = (!loadedFromSD) ? UIState::CHOOSE_PET : UIState::PET_SCREEN;

      if (!loadedFromSD) {
        g_app.inventory.init();
      }

      // FIRST BOOT: Controls Help -> wizard
      if (!loadedFromSD) {
        if (!g_controlsHelpSeen) {
          controlsHelpBegin(UIState::BOOT_WIFI_PROMPT, Tab::TAB_PET);
          ui_setBootSplashActive(false);
          return;
        }

        bootWizardBegin(afterOk, Tab::TAB_PET);
        ui_setBootSplashActive(false);
        return;
      }

      // NOT first boot (save exists): forced manual time path
      beginForcedSetTimeBootGate(afterOk, Tab::TAB_PET);

      g_app.uiState    = UIState::SET_TIME;
      g_app.currentTab = Tab::TAB_PET;
      requestUIRedraw();

      ui_setBootSplashActive(false);

      invalidateBackgroundCache();
      requestUIRedraw();
      renderUI();
      clearInputLatch();
      return;
    }

    // -----------------------------------------------------------------------
    // No save exists -> new pet flow
    // -----------------------------------------------------------------------
    uint16_t seedMarkNow = 0;
    EEPROM.get(SEED_MARK_ADDR, seedMarkNow);

    if (!loadedFromSD) {
      DBG_ON("[BOOT] No SD save -> UIState::CHOOSE_PET\n");

      g_app.inventory.init();

      if (!g_controlsHelpSeen) {
        beginForcedSetTimeBootGate(UIState::CHOOSE_PET, Tab::TAB_PET);
        controlsHelpBegin(UIState::SET_TIME, Tab::TAB_PET);
        ui_setBootSplashActive(false);
        return;
      }

      g_app.uiState    = UIState::CHOOSE_PET;
      g_app.currentTab = Tab::TAB_PET;
      uiInitLevelPopupTracker();
      requestUIRedraw();

      ui_setBootSplashActive(false);

      invalidateBackgroundCache();
      requestUIRedraw();
      renderUI();
      return;
    }

    if (seedMarkNow != SEED_MARK) {
      EEPROM.put(SEED_MARK_ADDR, (uint16_t)SEED_MARK);
      EEPROM.commit();
    }

    if (g_app.uiState == UIState::BOOT) {
     g_app.uiState    = UIState::PET_SCREEN;
     g_app.currentTab = Tab::TAB_PET;
      requestUIRedraw();
    }

    ui_setBootSplashActive(false);

    invalidateBackgroundCache();
    requestUIRedraw();
    renderUI();
  }

  // ---------------------------------------------------------------------------
  // Stage 3: WiFi/NTP init (deferred)
  // ---------------------------------------------------------------------------
  if (!g_postBootInitDone) {
    if (now >= g_nextWifiTryMs) {
      g_nextWifiTryMs = now + 1000;

      if (!g_wifiApplied) {
        const bool pref = settingsWifiEnabled();
        wifiSetEnabled(pref);
        applyWifiPower(pref);
        g_wifiApplied = true;
      }

      wifiTimeInit();
      g_postBootInitDone = true;
      requestUIRedraw();
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // Stage 4: Persist time anchor once we have synced time
  // ---------------------------------------------------------------------------
  if (!g_ntpSaved && timeIsSynced()) {
    g_ntpSaved = true;
    saveTimeAnchor();
  }
}
