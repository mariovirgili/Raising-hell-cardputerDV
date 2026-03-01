#include "flow_controls_help.h"

#include "app_state.h"
#include "controls_help_state.h"
#include "ui_actions.h"
#include "ui_defs.h"
#include "input.h"

// Public entry points used by app hotkeys / settings menu
void openControlsHelpFromSettings()
{
  // Return to Settings and preserve the current tab.
  controlsHelpBegin(UIState::SETTINGS, g_app.currentTab);
}

void openControlsHelpFromAnywhere()
{
  // Return to wherever we are right now, preserving the current tab.
  controlsHelpBegin(g_app.uiState, g_app.currentTab);
}

// Controls help UIState handler (routed by ui_input_router)
void uiControlsHelpHandle(InputState& in)
{
  
// Dismiss on ENTER/SELECT (and optionally MENU/BACK), not ESC.
// This prevents ESC from skipping required flows/screens.
if (in.selectOnce || in.menuOnce)
{
  // Swallow this frame's input so it doesn't leak into the next screen.
  uiActionSwallowAll(in);

  controlsHelpDismiss();
  return;
}


  // No other interactions needed for this help screen.
}