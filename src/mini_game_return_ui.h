#pragma once

#include "ui_defs.h"  // UIState

void   miniGameSetReturnUi(UIState s);
void   miniGameClearReturnUi();
UIState miniGameGetReturnUiOrDefault(UIState fallback);