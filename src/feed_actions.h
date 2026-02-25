#pragma once
#include "item.h"   // for ItemType

// Low-level helpers
bool consumeOneSoulFood();
int  consumeSoulFoodUntilFull();

// UI action (used by the pet screen feed menu)
void feedUseSelected();