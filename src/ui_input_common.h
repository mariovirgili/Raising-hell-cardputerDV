#pragma once

struct InputState;

// Existing helpers (already used in your codebase)
void uiDrainKb(InputState& in);
void uiSwallowTypingAndEdges(InputState& in);

// -----------------------------------------------------------------------------
// New unified “semantic input” helpers (Phase 2)
// These do NOT change behavior — they just centralize the common patterns.
// -----------------------------------------------------------------------------
int  uiNavMove(const InputState& in);           // encoderDelta with up/down override
bool uiSelectPressed(const InputState& in);     // selectOnce OR encoderPressOnce
bool uiBackPressed(const InputState& in);       // menuOnce OR escOnce (not used everywhere yet)

void uiWrapIndex(int& idx, int count);          // wrap 0..count-1