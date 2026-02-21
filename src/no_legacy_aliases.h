#pragma once

// If anyone re-introduces legacy alias globals, fail fast.
#ifdef uiState
#error "Do not use uiState alias; use g_app.uiState"
#endif
#ifdef currentTab
#error "Do not use currentTab alias; use g_app.currentTab"
#endif
