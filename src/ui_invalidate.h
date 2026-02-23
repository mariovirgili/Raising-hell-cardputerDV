#pragma once

// UI invalidation / redraw requests (small include surface).
void requestUIRedraw();

// Consume (read + clear) the redraw flag in a single place.
// Returns true if a redraw was requested.
bool consumeUIRedrawRequest();