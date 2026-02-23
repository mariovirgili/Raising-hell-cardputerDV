#pragma once
#include "mini_game_state.h"
#include <Arduino.h>
#include "state_manager_fwd.h"
#include "mini_games.h"    

class GameState;

class ResurrectionRunGameState : public MiniGameState {
public:
    void enter() override;
    void exit() override;
    void update() override;
};