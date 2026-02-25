#pragma once

#include <stdint.h>
#include "ui_defs.h"

struct InputState;

// Redraw helpers
void uiActionRequestRedraw();
void uiActionRequestFullRedraw();

// Input swallowing helpers
void uiActionDrainKb(InputState& in);
void uiActionSwallowEdges(InputState& in);
void uiActionSwallowAll(InputState& in);

// State transitions
void uiActionEnterState(UIState state, Tab tab, bool redraw = true);