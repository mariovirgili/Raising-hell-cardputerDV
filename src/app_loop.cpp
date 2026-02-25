#include "app_loop.h"

#include "M5Cardputer.h"
#include "anim_engine.h"
#include "app_state.h"
#include "auto_screen.h"
#include "boot_pipeline.h"
#include "build_flags.h"
#include "console.h"
#include "debug_state.h"
#include "display.h"
#include "display_state.h"
#include "evolution_flow.h"
#include "game_options_state.h"
#include "graphics.h"
#include "hatching_flow.h"
#include "input.h"
#include "input_activity_state.h"
#include "led_status.h"
#include "menu_actions.h"
#include "motion.h"
#include "no_legacy_aliases.h"
#include "pet.h"
#include "power_button.h"
#include "save_manager.h"
#include "sdcard.h"
#include "sleep_state.h"
#include "sound.h"
#include "time_persist.h"
#include "time_state.h"
#include "ui_level_popup.h"
#include "ui_runtime.h"
#include "ui_tabs.h"
#include "wifi_time.h"
#include <Arduino.h>
#include <cstring>
#include "flow_controls_help.h"

bool handleMenuInput(InputState &in);

static bool s_forcedFirstRender = false;
static uint32_t s_hbNextMs = 0;
static bool s_bootKeepAwakeInited = false;
static uint32_t s_bootKeepAwakeUntilMs = 0;
static bool s_prevSleeping = false;

void appServicesTick(uint32_t nowMs)
{
  M5Cardputer.update();
  pollDebugPort();
  keyboardDebugTick();
  powerButtonTick(nowMs);
  postBootInitTick();
  soundTick();
}

void appMainLoopTick()
{

  // ---------------------------------------------------------------------------
  // TRUE LOOP-ENTRY TIMESTAMP
  // ---------------------------------------------------------------------------
  const uint32_t now = millis();

  // ---------------------------------------------------------------------------
  // SERIAL SAFE PRINT HELPERS
  // ---------------------------------------------------------------------------
  auto dbgCanWrite = [&](size_t need) -> bool
  {
    if (!g_debugEnabled)
      return false;
    return Serial.availableForWrite() >= (int)need;
  };

  auto dbgPrintln = [&](const char *s)
  {
    if (!g_debugEnabled)
      return;
    const size_t need = strlen(s) + 2;
    if (dbgCanWrite(need))
      Serial.println(s);
  };

  auto dbgPrintf = [&](const char *fmt, auto... args)
  {
    if (!g_debugEnabled)
      return;
    if (!dbgCanWrite(128))
      return;
    Serial.printf(fmt, args...);
  };

  // ---------------------------------------------------------------------------
  // MAIN LOOP
  // ---------------------------------------------------------------------------
  appServicesTick(now);

  // ---------------------------------------------------------------------------
  // SCREEN OFF PATH
  // ---------------------------------------------------------------------------
  if (!isScreenOn())
  {
    wifiTimeTick();
    updateTime();
    updateBattery();
    saveManagerTick();
    maybePeriodicTimeSave();

    const bool sleepingNow_off = isPetSleepingNow();

    if (sleepingNow_off)
    {
      pet.petSleepTick();
      petResetUpdateTimers(); // prevent decay "catch-up" on wake
    }
    else
    {
      pet.update();
    }

    // Near-death beep MUST work even with screen off.
    soundLowHealthTick((uint8_t)pet.health, sleepingNow_off,
                       /*screenOn=*/false,
                       /*inDeathScreen=*/(g_app.uiState == UIState::DEATH));

    if (motionAvailable && motionShakeDetected())
    {
      SET_SCREEN_POWER(true);
      noteUserActivity();
      invalidateBackgroundCache();
      requestUIRedraw();
      clearInputLatch();
    }

#if LED_STATUS_ENABLED
    ledSetScreenOff(true);
    ledUpdatePetStatus(computeLedMode());
#endif

    delay(5);
    return;
  }

  // ---------------------------------------------------------------------------
  // SCREEN ON PATH
  // ---------------------------------------------------------------------------
  // ---------------------------------------------------------------------------
  // Force one render after boot so we never sit on a blank screen because
  // uiNeedsRedraw was never set by the boot pipeline.
  // ---------------------------------------------------------------------------
  if (!s_forcedFirstRender)
  {
    s_forcedFirstRender = true;
    noteUserActivity();
    invalidateBackgroundCache();
    requestUIRedraw();
    renderUI();
  }

  InputState input = readInput();

  // ---------------------------------------------------------------------------
  // MODAL: SD assets missing screen (all builds)
  // Draw ONCE to prevent flicker; retry check on ENTER.
  // ---------------------------------------------------------------------------
  if (g_assetsMissing)
  {
    static bool s_prevSelectHeld_assets = false;
    const bool enterOnce = (input.selectHeld && !s_prevSelectHeld_assets);
    s_prevSelectHeld_assets = input.selectHeld;

    static bool s_drawn = false;
    if (!s_drawn)
    {
      drawAssetsMissingScreen();
      s_drawn = true;
    }

    if (enterOnce || input.selectOnce)
    {
      // Re-check SD + marker file
      if (!g_sdReady)
      {
        g_sdReady = initSD();
      }

      // If SD comes back, allow the normal boot pipeline to proceed again.
      if (g_sdReady)
      {
        g_sdGaveUp = false;
        g_sdFirstTryMs = 0;
        g_sdTryCount = 0;
      }

      g_assetsMissing = !(g_sdReady && sdAssetsPresent());
      g_assetsChecked = true;

      if (!g_assetsMissing)
      {
        s_drawn = false;
        drawBootSplash();
        invalidateBackgroundCache();
        requestUIRedraw();
        renderUI();
      }
      else
      {
        s_drawn = false; 
      }
    }

    soundTick();
    delay(10);
    return;
  }

  // ---------------------------------------------------------------------------
  // LEVEL UP MODAL (blocks input until dismissed with ENTER or G)
  // ---------------------------------------------------------------------------
  {
    static bool s_prevSelectHeld_levelUp = false;
    const bool enterOnce = (input.selectHeld && !s_prevSelectHeld_levelUp);
    s_prevSelectHeld_levelUp = input.selectHeld;

    if (uiIsLevelUpPopupActive())
    {
      if (enterOnce || input.selectOnce)
      {
        uiDismissLevelUpPopup();
        clearInputLatch();
        requestUIRedraw();
      }
      else
      {
        requestUIRedraw();
      }

      if (consumeUIRedrawRequest())
      {
        renderUI();
      }

      wifiTimeTick();
      if (g_timeAnchorAttempted || timeIsSynced())
        updateTime();
      updateBattery();
      saveManagerTick();
      maybePeriodicTimeSave();

#if LED_STATUS_ENABLED
      ledSetScreenOff(false);
      ledUpdatePetStatus(computeLedMode());
#endif
      return;
    }
  }

  // ---------------------------------------------------------------------------
  // FAST TAB SWITCH PATH (apply state immediately, do NOT render here)
  // ---------------------------------------------------------------------------
  {
    const bool allowTabLR_fast = (g_app.uiState == UIState::PET_SCREEN) || (g_app.uiState == UIState::SLEEP_MENU) ||
                                 (g_app.uiState == UIState::INVENTORY) || (g_app.uiState == UIState::SHOP);

    if (allowTabLR_fast && (input.leftOnce || input.rightOnce))
    {
      if (input.leftOnce)
        tabPrev();
      if (input.rightOnce)
        tabNext();

      noteUserActivity();
      clearInputLatch();

      invalidateBackgroundCache();

      syncUiToTab();
      requestUIRedraw();
      soundClick();
      return;
    }
  }

  if (g_app.uiState != UIState::CONSOLE)
  {
    if (input.upOnce || input.downOnce || (input.encoderDelta != 0))
      soundMenuTick();
    if (input.leftOnce || input.rightOnce)
      soundClick();
    if (input.selectOnce || input.encoderPressOnce)
      soundConfirm();
    if (input.menuOnce || input.escOnce)
      soundCancel();
  }

    // AUTO SCREEN
    if (hasUserActivity(input))
      noteUserActivity();

    // ---------------------------------------------------------------------------
    // BOOT KEEP-AWAKE (prevents auto_screen from blanking during early init)
    // ---------------------------------------------------------------------------
    if (!s_bootKeepAwakeInited)
    {
      s_bootKeepAwakeInited = true;
      s_bootKeepAwakeUntilMs = now + 6000;
    }

    if ((int32_t)(now - s_bootKeepAwakeUntilMs) < 0)
    {
      // Feed the project's inactivity timer + ensure we render at least once.
      noteUserActivity();
      requestUIRedraw();

      // If something already blanked the screen during boot, force it back on.
      if (!isScreenOn())
      {
        SET_SCREEN_POWER(true);
        invalidateBackgroundCache();
        requestUIRedraw();
        clearInputLatch();
      }
    }

    autoScreenTick();

    if (!isScreenOn())
    {
#if LED_STATUS_ENABLED
      ledSetScreenOff(true);
      ledUpdatePetStatus(computeLedMode());
#endif
      delay(5);
      return;
    }

    // DEATH/BURIAL special flow 
    if (g_app.uiState == UIState::DEATH)
    {
      const UIState before = g_app.uiState;
      handleMenuInput(input);

      if (g_app.uiState != before)
      {
        g_app.uiNeedsRedraw = true;
        renderUI();
        return;
      }

      if (input.upOnce || input.downOnce || input.selectOnce || input.encoderPressOnce || (input.encoderDelta != 0))
      {
        requestUIRedraw();
      }

      renderUI();
      return;
    }

    if (g_app.uiState == UIState::BURIAL_SCREEN)
    {
      handleMenuInput(input);
      if (input.selectOnce || input.encoderPressOnce)
        requestUIRedraw();
      renderUI();
      return;
    }

    // AUTO-RETURN TO PET TAB
    if (g_app.uiState == UIState::PET_SCREEN && g_app.currentTab != Tab::TAB_PET)
    {
      const uint32_t nowMs = millis();
      if ((uint32_t)(nowMs - getLastInputActivityMs()) >= 60000UL)
      {
        g_app.currentTab = Tab::TAB_PET;
        requestUIRedraw();
        clearInputLatch();
      }
    }

    // ---------------------------------------------------------------------------
    // HOTKEYS: Console + Settings (must run BEFORE handleMenuInput)
    // ---------------------------------------------------------------------------

    // SET TIME: lock out global hotkeys so the editor can't be bypassed
    if (g_app.uiState == UIState::SET_TIME)
    {
      input.tabJump = 255;
      input.consoleOnce = false;
      input.hotSettings = false;
    }

    // If sleeping, block focus-stealing tab hotkeys.
    const bool sleepingNow = isPetSleepingNow();
    if (sleepingNow)
    {
      input.tabJump = 255;

      if (g_app.uiState == UIState::PET_SLEEPING)
      {
        input.upOnce = false;
        input.downOnce = false;
        input.leftOnce = false;
        input.rightOnce = false;
      }
    }

    // Don't allow ESC/C/Q to steal focus on New Pet flow screens
    if (g_app.uiState == UIState::CHOOSE_PET)
    {
      input.consoleOnce = false;
      input.escOnce = false;
      input.hotSettings = false;
      input.menuOnce = false;
      input.tabJump = 255;
    }
    else
    {
      // Bottom-row tab hotkeys (z x c v b n m) — only when not in restricted screens
      if (g_app.uiState != UIState::NAME_PET && g_app.uiState != UIState::SET_TIME)
      {

        if (sleepingNow && input.tabJump != 255)
        {
          input.tabJump = 255;
          clearInputLatch();
        }

        // Don't allow ESC/C/Q/tab jumps to steal focus during Hatching/Evolution
        if (g_app.uiState == UIState::HATCHING || g_app.flow.evo.active || g_app.uiState == UIState::EVOLUTION)
        {
          input.tabJump = 255;
          input.consoleOnce = false;
          input.escOnce = false;
          input.hotSettings = false;
          input.menuOnce = false;
        }

        if (input.tabJump != 255)
        {
          noteUserActivity();

          g_app.currentTab = (Tab)input.tabJump;
          syncUiToTab();

          invalidateBackgroundCache();
          requestUIRedraw();
          clearInputLatch();
          return;
        }
      }

#if !PUBLIC_BUILD
      // C toggles console
      if (g_app.uiState != UIState::SET_TIME && input.consoleOnce)
      {
        noteUserActivity();
  
        if (g_app.uiState != UIState::CONSOLE)
        {
          consoleOpen();
          g_app.uiState = UIState::CONSOLE;
        }
        else
        {
          consoleClose();
          if (sleepingNow)
            g_app.uiState = UIState::PET_SLEEPING;
          else
          {
            g_app.uiState = UIState::PET_SCREEN;
            g_app.currentTab = Tab::TAB_PET;
          }
        }
  
        invalidateBackgroundCache();
        requestUIRedraw();
        // Clear ALL edge flags so nothing leaks into the new state this frame
        input = InputState{};
        clearInputLatch();
        return;
      }

      // I opens Controls help overlay (non-interrupting, including pet sleep)
      if (g_app.uiState != UIState::SET_TIME && input.controlsOnce)
      {
        noteUserActivity();
        openControlsHelpFromAnywhere();
        return;
      }
#endif
    }

    // Q/menu key returns to pet tab/root
    // IMPORTANT: do NOT run this while POWER_MENU is open (it steals menuOnce
    // from handlePowerMenuInput)
    if (g_app.uiState != UIState::SET_TIME && g_app.uiState != UIState::POWER_MENU && input.menuOnce)
    {

      // While sleeping, do NOT hijack menuOnce here.
      // Let handleMenuInput() decide.
      if (g_app.uiState != UIState::PET_SLEEPING)
      {
        noteUserActivity();

        if (g_app.uiState != UIState::PET_SCREEN || g_app.currentTab != Tab::TAB_PET)
        {
          g_app.uiState = UIState::PET_SCREEN;
          g_app.currentTab = Tab::TAB_PET;

          invalidateBackgroundCache();
          requestUIRedraw();
          return;
        }
      }
    }

    // ---------------------------------------------------------------------------
    // Waking from sleep screen state
    // ---------------------------------------------------------------------------
    if (g_app.uiState == UIState::PET_SLEEPING && !isPetSleepingNow())
    {
      petResetUpdateTimers();
      g_app.uiState = UIState::PET_SCREEN;
      g_app.currentTab = Tab::TAB_PET;
      invalidateBackgroundCache();
      requestUIRedraw();
    }

    // ---------------------------------------------------------------------------
    // Input-driven redraw hint (single copy)
    // ---------------------------------------------------------------------------
    if (input.menuOnce || input.selectOnce || input.upOnce || input.downOnce || (input.encoderDelta != 0))
    {
      requestUIRedraw();
    }

    // ---------------------------------------------------------------------------
    // HATCHING: modal tick, then render, then return
    // ---------------------------------------------------------------------------
    if (g_app.uiState == UIState::HATCHING)
    {
      updateHatching();
      if (isScreenOn())
        requestUIRedraw();

      if (consumeUIRedrawRequest())
      {
        renderUI();
      }

      wifiTimeTick();
      if (g_timeAnchorAttempted || timeIsSynced())
        updateTime();
      updateBattery();
      saveManagerTick();
      maybePeriodicTimeSave();

#if LED_STATUS_ENABLED
      ledSetScreenOff(false);
      ledUpdatePetStatus(computeLedMode());
#endif
      return;
    }

    // ---------------------------------------------------------------------------
    // EVOLUTION: modal tick, then render, then return
    // ---------------------------------------------------------------------------
    if (g_app.flow.evo.active || g_app.uiState == UIState::EVOLUTION)
    {
      updateEvolution();
      if (isScreenOn())
        requestUIRedraw();

      if (consumeUIRedrawRequest())
      {
        renderUI();
      }
      wifiTimeTick();
      if (g_timeAnchorAttempted || timeIsSynced())
        updateTime();
      updateBattery();
      saveManagerTick();
      maybePeriodicTimeSave();

#if LED_STATUS_ENABLED
      ledSetScreenOff(false);
      ledUpdatePetStatus(computeLedMode());
#endif

      return;
    }

    // ---------------------------------------------------------------------------
    // Pet tick (ALWAYS run even if Console is open)
    // ---------------------------------------------------------------------------

    const bool inDeathFlow = (g_app.uiState == UIState::DEATH) || (g_app.uiState == UIState::MINI_GAME) ||
                             (g_app.uiState == UIState::BURIAL_SCREEN);

    if (!inDeathFlow)
    {
      if (isPetSleepingNow())
      {
        pet.petSleepTick();
        petResetUpdateTimers();
      }
      else
      {
        pet.update();

        // Detect level-ups right after the pet tick updates XP/level.
        uiMaybeShowLevelUpPopup();
      }

      if (pet.health <= 0 && petDeathEnabled && g_app.uiState != UIState::DEATH)
      {
        petEnterDeathState();
        invalidateBackgroundCache();
        requestUIRedraw();
        clearInputLatch();
      }
    }

    // ---------------------------------------------------------------------------
    // Menu input
    // ---------------------------------------------------------------------------
    handleMenuInput(input);

    const bool sleepingNow2 = isPetSleepingNow();

    if (!s_prevSleeping && sleepingNow2)
      soundSleep();
    if (s_prevSleeping && !sleepingNow2)
      soundWake();
    s_prevSleeping = sleepingNow2;

    soundLowHealthTick((uint8_t)pet.health, sleepingNow2,
                       /*screenOn=*/isScreenOn(),
                       /*inDeathScreen=*/(g_app.uiState == UIState::DEATH));

    // Anim tick
    if (g_sdReady)
    {
      animTick();
    }

    // Anim heartbeat (sleep background frames)
    sleepAnimHeartbeat(now);
    sleepMiniStatsHeartbeat(now);

    // Render
    if (consumeUIRedrawRequest())
    {
      renderUI();
    }
    // Maintenance AFTER render
    wifiTimeTick();
    if (g_timeAnchorAttempted || timeIsSynced())
      updateTime();
    updateBattery();
    saveManagerTick();
    maybePeriodicTimeSave();

#if LED_STATUS_ENABLED
    ledSetScreenOff(false);
    ledUpdatePetStatus(computeLedMode());
#endif
  }
