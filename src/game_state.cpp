#include "game_state.h"
#include "state_manager.h"
#include "flappy_fireball_game_state.h"
#include "pause_state.h"

void GameState::enter() {
    Serial.println("Entering GameState...");
    playerPet = Pet();
}

void GameState::exit() {
    Serial.println("Exiting GameState...");
}

void GameState::update() {
    static int gameTick = 0;
    gameTick++;

    if (gameTick % 100 == 0) {
        playerPet.update();
        Serial.print("Pet Status - Health: ");
        Serial.print(playerPet.health);
        Serial.print(", Happiness: ");
        Serial.println(playerPet.happiness);
    }

    if (gameTick % 200 == 0) {
        playerPet.feed(10);
        Serial.println("Pet fed!");
    }

    if (gameTick == 500) {
        Serial.println("Starting Flappy Fireball Mini-Game...");
        state_manager.setState(new FlappyFireballGameState());
    }

    if (gameTick > 1000) {
        state_manager.setState(new PauseState());
    }
}