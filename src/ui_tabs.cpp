#include "ui_tabs.h"

#include "app_state.h"
#include "ui_defs.h"
#include "ui_runtime.h"

static inline int tabCountInt() {
  return (int)Tab::TAB_COUNT;
}

void tabNext() {
  int t = (int)g_app.currentTab;
  const int n = tabCountInt();
  t = (t + 1) % n;
  g_app.currentTab = (Tab)t;
}

void tabPrev() {
  int t = (int)g_app.currentTab;
  const int n = tabCountInt();
  t = (t + n - 1) % n;
  g_app.currentTab = (Tab)t;
}

void syncUiToTab() {
  const UIState prev = g_app.uiState;

  switch (g_app.currentTab) {
    case Tab::TAB_PET:   g_app.uiState = UIState::PET_SCREEN; break;
    case Tab::TAB_SLEEP: g_app.uiState = UIState::SLEEP_MENU; break;
    case Tab::TAB_INV:   g_app.uiState = UIState::INVENTORY; break;
    case Tab::TAB_SHOP:  g_app.uiState = UIState::SHOP; break;
    default:             g_app.uiState = UIState::PET_SCREEN; break;
  }

  if (g_app.uiState != prev) {
    requestUIRedraw();
  }
}
