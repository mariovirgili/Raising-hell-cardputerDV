#include "pet_state.h"

#include <Arduino.h>
#include "pet.h"
#include "save_manager.h"

Pet pet;

static uint32_t g_resGraceUntilMs = 0;

void petResurrectFull() {
  pet.health     = 100;
  pet.hunger     = 100;
  pet.energy     = 100;
  pet.happiness  = 100;
  pet.isSleeping = false;

  g_resGraceUntilMs = millis() + 2000;

  petResetUpdateTimers();

  saveManagerMarkDirty();
}

bool petResurrectGraceActive() {
  return (int32_t)(g_resGraceUntilMs - millis()) > 0;
}
