#pragma once

struct InputState;

// Catalog info for UI/rendering
int uiShopMenuCount();
int uiShopMenuPrice(int idx);
const char* uiShopMenuLabel(int idx);

// Returns true if action ran (purchase attempted)
bool uiShopMenuActivate(int idx, InputState& in);

// Optional helper for UI (grey out, etc)
bool uiShopMenuCanAfford(int idx);