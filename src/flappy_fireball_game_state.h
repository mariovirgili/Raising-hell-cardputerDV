#pragma once
#include "mini_game_state.h"
#include <Arduino.h>
#include "state_manager_fwd.h"

class GameState;

class FlappyFireballGameState : public MiniGameState {
public:
    void enter() override;
    void exit() override;
    void update() override;
};