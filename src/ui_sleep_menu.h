#pragma once

// Sleep menu is a small, fixed set of options.
// Exposes labels + activation so input and rendering stay in sync.

int uiSleepMenuCount();
const char* uiSleepMenuLabel(int idx);
void uiSleepMenuActivate(int idx);