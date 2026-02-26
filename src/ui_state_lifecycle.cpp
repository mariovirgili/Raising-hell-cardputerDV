#include "ui_state_lifecycle.h"

#include "app_state.h"
#include "input.h"
#include "console.h"

static bool wantsTextCapture(UIState s)
{
  switch (s)
  {
    case UIState::CONSOLE:
    case UIState::NAME_PET:
    case UIState::WIFI_SETUP:
      return true;
    default:
      return false;
  }
}

void uiStateOnEnter(UIState s)
{
  // Enforce text capture mode based on state.
  const bool capture = wantsTextCapture(s);
  inputSetTextCapture(capture);
  g_textCaptureMode = capture;

  // State-specific “enter” side effects.
  switch (s)
  {
    case UIState::CONSOLE:
      // If something entered CONSOLE without calling consoleOpen(),
      // this at least guarantees the console is in the right mode.
      consoleOpen();
      break;

    default:
      break;
  }
}

void uiStateOnExit(UIState s)
{
  switch (s)
  {
    case UIState::CONSOLE:
      // Safety: if anything transitions away from CONSOLE without
      // going through uiConsoleHandle’s exit path, close properly.
      consoleClose();
      break;

    default:
      break;
  }
}