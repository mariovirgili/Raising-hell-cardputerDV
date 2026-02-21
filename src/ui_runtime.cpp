#include "ui_runtime.h"
#include "graphics.h"
#include "ui_invalidate.h"

UIRuntimeState g_ui;

void requestFullUIRedraw() {
  invalidateBackgroundCache();
  requestUIRedraw();
}
