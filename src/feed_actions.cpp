#include "feed_actions.h"

#include <Arduino.h>

#include "app_state.h"
#include "graphics.h"      // ui_showMessage
#include "pet.h"
#include "save_manager.h"
#include "ui_menu_state.h" // feedMenuIndex

// Apply Soul Food effect + consume exactly one item.
// Returns true only if an item was available and used.
bool consumeOneSoulFood() {
  if (!g_app.inventory.hasItem(ITEM_SOUL_FOOD)) return false;

  pet.hunger    = constrain(pet.hunger + 20, 0, 100);
  pet.happiness = constrain(pet.happiness + 10, 0, 100);
  pet.energy    = constrain(pet.energy + 10, 0, 100);

  g_app.inventory.removeItem(ITEM_SOUL_FOOD, 1);

  pet.lastFedTime = millis();
  saveManagerMarkDirty();
  return true;
}

int consumeSoulFoodUntilFull() {
  int used = 0;
  while (pet.hunger < 100) {
    if (!consumeOneSoulFood()) break;
    used++;
    if (used > 99) break; // safety
  }
  return used;
}

void feedUseSelected() {
  if (feedMenuIndex == 0) {
    if (pet.hunger >= 100) {
      ui_showMessage("Already full!");
      return;
    }

    if (!consumeOneSoulFood()) ui_showMessage("No Soul Food!");
    else ui_showMessage("Snack time!");
    return;
  }

  if (feedMenuIndex == 1) {
    if (pet.hunger >= 100) {
      ui_showMessage("Already full!");
      return;
    }

    int used = consumeSoulFoodUntilFull();
    if (used <= 0) ui_showMessage("No Soul Food!");
    else {
      String msg = "Ate x" + String(used);
      ui_showMessage(msg.c_str());
    }
    return;
  }
}