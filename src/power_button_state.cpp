// power_button_state.cpp
#include "power_button_state.h"
#include "ui_invalidate.h"   

bool screenButtonHeld = false;
uint32_t screenButtonPressTime = 0;
bool screenJustWentOff = false;

bool inPowerMenu = false;
