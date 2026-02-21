#include <Arduino.h>
#include <cstdio>

#include "app_state.h"

#include "inventory.h"
#include "inventory_state.h"

#include "currency.h"
#include "ui_runtime.h"
#include "shop_items.h"
#include "save_manager.h"

#include "ui_runtime.h"
#include "graphics.h"
#include "app_state.h"

// Shop Index Mapping:
// 0 = Soul Food
// 1 = Cursed Relic
// 2 = Demon Bone
// 3 = Ritual Chalk
// 4 = Eldritch Eye
// 5 = Exit

bool shopBuyItem()
{
  if (SHOP_ITEM_COUNT <= 0) return false;

if (g_app.shopIndex < 0) g_app.shopIndex = 0;
if (g_app.shopIndex >= SHOP_ITEM_COUNT) g_app.shopIndex = SHOP_ITEM_COUNT - 1;

const ShopItem& it = availableItems[g_app.shopIndex];

  if (!spendInf(it.price)) {
    ui_showMessage("Not enough INF!");
    return false;
  }

  g_app.inventory.addItem(it.type, 1);
  saveManagerMarkDirty();
  char msg[48];
  const char* nm = g_app.inventory.getItemLabelForType(it.type);
  if (!nm) nm = "";

  snprintf(msg, sizeof(msg), "Purchased %s", nm);  ui_showMessage(msg);  return true;
}
