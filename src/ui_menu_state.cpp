// ui_menu_state.cpp
#include "ui_menu_state.h"
#include "app_state.h"

// Aliases into g_app (single source of truth)
int&      shopIndex        = g_app.shopIndex;
int&      feedMenuIndex    = g_app.feedMenuIndex;
int&      powerMenuIndex   = g_app.powerMenuIndex;
uint32_t& lastRenderTimeMs = g_app.lastRenderTimeMs;

// Migrated legacy indices: also live in g_app now
int& sleepMenuIndex = g_app.sleepMenuIndex;
int& playMenuIndex  = g_app.playMenuIndex;
int& selectedMenu   = g_app.selectedMenu;
