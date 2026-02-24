#pragma once

#include <stdint.h>
#include "ui_defs.h"

struct InputState;

namespace UiSettingsMenu {

  // Handles the current SettingsPage if it is data-driven.
  // Returns true if the page was handled, false if caller should fall back to legacy handlers.
  bool Handle(InputState& input, int move);

} // namespace UiSettingsMenu