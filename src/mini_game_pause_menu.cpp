/*
  LEGACY FILE — DO NOT COMPILE
  Mini-game implementation lives in src/mini_games.cpp
*/

#include "build_flags.h"

// If someone flips the build to use pause-menu impl, force them to do real work.
#if RH_MINIGAMES_IMPL_IN_PAUSE_MENU
  #error "RH_MINIGAMES_IMPL_IN_PAUSE_MENU=1 but mini_game_pause_menu.cpp is a dead file. Move the implementation here OR set RH_MINIGAMES_IMPL_IN_PAUSE_MENU=0."
#endif

// Otherwise: intentionally empty.