#include "shop_items.h"

// IMPORTANT: order MUST match graphics.cpp shopItemTypeForIndexLocal():
// 0 Soul Food, 1 Cursed Relic, 2 Demon Bone, 3 Ritual Chalk, 4 Eldritch Eye
const ShopItem availableItems[] = {
  { ITEM_SOUL_FOOD,     20 },
  { ITEM_CURSED_RELIC,  20 },
  { ITEM_DEMON_BONE,    20 },
  { ITEM_RITUAL_CHALK,  50 },
  { ITEM_ELDRITCH_EYE,  200 },
};

const int SHOP_ITEM_COUNT = (int)(sizeof(availableItems) / sizeof(availableItems[0]));
