#pragma once
#include <stdbool.h>

// Gate to prevent instant hatch when entering CHOOSE_PET while ENTER is held
// or stale selectOnce leaks in from the prior screen.
extern bool g_choosePetBlockHatchUntilRelease;
