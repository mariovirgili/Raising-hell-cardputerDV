#include "ui_input_router.h"

#include "app_state.h"
#include "input.h"

#include "ui_input_interceptors.h"

#include "ui_actions.h"

// Central state dispatch table
#include "ui_state_handlers.h"

bool uiInputApplyInterceptors(InputState& in);

static inline bool wantsTextCapture(UIState s)
{
  switch (s)
  {
  case UIState::CONSOLE:
  case UIState::WIFI_SETUP:
  case UIState::NAME_PET:
    return true;
  default:
    return false;
  }
}

static inline bool isNoInputState(UIState s)
{
  switch (s)
  {
  case UIState::BOOT:
    return true;
  default:
    return false;
  }
}

static bool dispatchToHandler(UIState s, InputState &in)
{
  return uiDispatchToStateHandler(s, in);
}


bool uiHandleInput(InputState &in)
{
  static bool s_lastTextCapture = false;

  const bool desiredTextCapture = wantsTextCapture(g_app.uiState);
  if (desiredTextCapture != s_lastTextCapture)
  {
    inputSetTextCapture(desiredTextCapture);
    s_lastTextCapture = desiredTextCapture;
  }

  // NOTE:
  // We no longer special-case console/name/wifi dispatch here.
  // Text-capture is already toggled above (inputSetTextCapture), and the
  // per-state handlers can decide what ESC/MENU does.
  // Keeping all dispatch in one place avoids "wrong handler" bugs.

  // Phase 1: global interceptors (power menu, ESC->settings, sleep gate, etc.)
  if (uiInputApplyInterceptors(in))
    return true;

  if (isNoInputState(g_app.uiState))
  {
    uiActionSwallowEdges(in);
    return true;
  }

  // Phase 2: per-state handler
  return dispatchToHandler(g_app.uiState, in);
}