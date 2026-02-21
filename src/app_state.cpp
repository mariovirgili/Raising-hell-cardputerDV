#include "app_state.h"
#include "ui_invalidate.h"

// Single definition of the application state.
// Keep this as aggregate default construction so GCC doesn't complain about
// designated-initializer ordering vs struct declaration order.
AppState g_app{};

void requestUIRedraw() {
  g_app.uiNeedsRedraw = true;
}
