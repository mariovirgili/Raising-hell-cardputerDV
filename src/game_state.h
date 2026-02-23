#pragma once
#include "state.h"
#include "pet.h"
#include "mini_game_state.h"
#include <Arduino.h>
#include "state_manager_fwd.h"

class FlappyFireballGameState;
class PauseState;

class GameState : public State {
    private:
        Pet playerPet;
        uint32_t lastPetUpdateMs = 0;
        uint32_t lastFeedMs = 0;
    public:
        void enter() override;
        void exit() override;
        void update() override;
    };