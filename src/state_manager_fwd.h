#pragma once
// Forward declaration only — do NOT include state_manager.h from state headers.
// This breaks the circular include chain:
//   state_manager.h → game_state.h → flappy_fireball_game_state.h → pause_state.h
//                                                                    → state_manager.h  ← LOOP
class StateManager;
extern StateManager state_manager;