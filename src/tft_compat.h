#pragma once

#if defined(ARDUINO_M5Cardputer) || defined(ARDUINO_M5STACK_CARDPUTER) || defined(M5_CARDPUTER)
#include "M5Cardputer.h"
extern lgfx::LGFX_Device& tft;
#endif
