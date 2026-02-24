#include "new_pet_flow_state.h"

bool    g_choosePetBlockHatchUntilRelease = false;
PetType g_pendingPetType                 = PET_DEVIL;
bool    g_namePetJustOpened              = false;