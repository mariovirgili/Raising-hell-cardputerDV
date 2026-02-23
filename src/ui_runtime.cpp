#include "ui_runtime.h"
#include "ui_invalidate.h"
#include "app_state.h"
#include "graphics.h"

void requestFullUIRedraw() {
  invalidateBackgroundCache();
  requestUIRedraw();
}