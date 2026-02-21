#include "tft_compat.h"

#include "display.h"

#if defined(ARDUINO_M5Cardputer) || defined(ARDUINO_M5STACK_CARDPUTER) || defined(M5_CARDPUTER)
#include "M5Cardputer.h"

// Some of your code still expects a global named `tft`.
// On Cardputer, redirect that name to the real display object.
lgfx::LGFX_Device& tft = M5Cardputer.Display;
#endif
