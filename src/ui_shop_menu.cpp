#include "ui_shop_menu.h"

#include "app_state.h"
#include "currency.h"
#include "input.h"
#include "shop_actions.h"
#include "shop_items.h"

int uiShopMenuCount()
{
  return SHOP_ITEM_COUNT;
}

int uiShopMenuPrice(int idx)
{
  if (idx < 0 || idx >= SHOP_ITEM_COUNT) return 0;
  return availableItems[idx].price;
}

const char* uiShopMenuLabel(int idx)
{
  if (idx < 0 || idx >= SHOP_ITEM_COUNT) return "";

  const ItemType t = availableItems[idx].type;
  const char* nm = g_app.inventory.getItemLabelForType(t);
  return nm ? nm : "";
}

bool uiShopMenuCanAfford(int idx)
{
  if (idx < 0 || idx >= SHOP_ITEM_COUNT) return false;
  return getInf() >= availableItems[idx].price;
}

bool uiShopMenuActivate(int idx, InputState& in)
{
  (void)in;
  if (idx < 0 || idx >= SHOP_ITEM_COUNT) return false;
  return shopBuyItem(idx);
}