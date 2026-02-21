// factory_reset_state.h
#pragma once

// Factory Reset confirmation overlay state.
// This used to live in g_ui (ui_runtime.h). Keep it isolated so UI buckets shrink.

struct FactoryResetState {
  bool confirmActive = false; // showing the confirm overlay
  int  confirmIndex  = 0;     // 0 = NO, 1 = YES
};

extern FactoryResetState g_factoryReset;
