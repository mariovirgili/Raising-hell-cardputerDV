#pragma once

#include <Arduino.h>
#include "ui_defs.h"
#include "pet_flow_state.h"
#include "inventory.h"
#include "ui_invalidate.h"

// Central app-wide runtime state.
// Keep this as the home for "global but not gameplay" UI/runtime flags.
struct AppState {
  // Display / rendering
  bool     screenOn = true;
  uint32_t lastRenderTimeMs = 0;
  bool     uiNeedsRedraw = true;
  PetFlowState flow;
  Inventory inventory;

  // Primary UI routing
  UIState  uiState = UIState::BOOT;
  Tab      currentTab = Tab::TAB_PET;

  // Legacy UI flags (still used by a few call sites)
  bool inShopMenu = false;
  bool inMiniGame = false;

  // Common menu cursors
  int settingsIndex = 0;
  int shopIndex = 0;
  int feedMenuIndex = 0;
  int powerMenuIndex = 0;

  // Legacy menu cursors (migrated from ui_menu_state.cpp)
  int sleepMenuIndex = 0;
  int playMenuIndex  = 0;
  int selectedMenu   = 0;
};

extern AppState g_app;
