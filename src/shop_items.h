#pragma once
#include "inventory.h"   // ItemType

// Shop catalog entry: UI uses index -> entry -> price/type.
// Names/descriptions come from Inventory label/desc helpers.
struct ShopItem {
  ItemType type;
  int      price;
};

extern const ShopItem availableItems[];
extern const int SHOP_ITEM_COUNT;
