#pragma once

#include <stdint.h>
#include "ui_defs.h"

enum class SettingsPage : uint8_t;

// Console flow entry that snapshots a return destination
void openConsoleWithReturn(UIState returnState,
                           Tab returnTab,
                           bool retToSettings,
                           SettingsPage retSettingsPage);