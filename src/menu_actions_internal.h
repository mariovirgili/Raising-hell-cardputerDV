// menu_actions_internal.h
#pragma once

#include <stdint.h>

// After leaving overlays (Controls Help / Console), suppress menu/esc edges briefly
void menuActionsSuppressMenuForMs(uint32_t durationMs);
