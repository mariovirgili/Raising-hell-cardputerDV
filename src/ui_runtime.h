#pragma once

// UIRuntimeState has been fully migrated into g_app (app_state.h).
// This file is kept as a stub so existing includes don't need to change.
// It provides requestFullUIRedraw() which many files legitimately need.

void requestFullUIRedraw();