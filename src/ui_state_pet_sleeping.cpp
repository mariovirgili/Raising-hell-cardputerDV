#include "ui_state_pet_sleeping.h"
#include "menu_actions.h" // handleSleepScreenInput

void uiPetSleepingHandle(InputState& in)
{
  // Transitional: reuse proven legacy behavior during refactor.
  handleSleepScreenInput(in);
}