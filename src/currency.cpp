#include "currency.h"

#include "pet.h"
#include "save_manager.h"

// ------------------------------------------------------------
// INF (Inferium) — stored in Pet (single source of truth)
// ------------------------------------------------------------
int getInf() {
  return pet.inf;
}

// Add or subtract INF (clamped). Negative allowed.
void addInf(int amount) {
  if (amount == 0) return;

  long v = (long)pet.inf + (long)amount;

  if (v < 0) v = 0;
  if (v > 999999) v = 999999;

  if ((int)v == pet.inf) return;
  pet.inf = (int)v;

  saveManagerMarkDirty();
}

bool spendInf(int amount) {
  if (amount <= 0) return true;
  if (pet.inf < amount) return false;

  pet.inf -= amount;
  saveManagerMarkDirty();
  return true;
}
