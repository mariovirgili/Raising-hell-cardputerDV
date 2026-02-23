#include "flappy_fireball_game_state.h"
#include "state_manager.h"
#include "game_state.h"

void FlappyFireballGameState::enter() {
    Serial.println("Entering Flappy Fireball Mini-Game...");
}

void FlappyFireballGameState::exit() {
    Serial.println("Exiting Flappy Fireball Mini-Game...");
}

void FlappyFireballGameState::update() {
    static int gameTick = 0;
    gameTick++;

    if (gameTick % 10 == 0) {
        Serial.print("Flappy Fireball update, tick: ");
        Serial.println(gameTick);

        if (gameTick > 100) {
            state_manager.setState(new GameState());
        }
    }
}