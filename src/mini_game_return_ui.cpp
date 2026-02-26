#include "mini_game_return_ui.h"

static bool    s_hasReturn = false;
static UIState s_returnUi  = UIState::PET_SCREEN;

void miniGameSetReturnUi(UIState s)
{
  s_hasReturn = true;
  s_returnUi  = s;
}

void miniGameClearReturnUi()
{
  s_hasReturn = false;
}

UIState miniGameGetReturnUiOrDefault(UIState fallback)
{
  return s_hasReturn ? s_returnUi : fallback;
}