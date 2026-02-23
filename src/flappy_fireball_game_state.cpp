#include "flappy_fireball_game_state.h"
#include "state_manager.h"
#include "game_state.h"

void FlappyFireballGameState::enter() {
    Serial.println("Entering Flappy Fireball Mini-Game...");
    startFlappyFireball();    // ← launches the real game
}

void FlappyFireballGameState::exit() {
    Serial.println("Exiting Flappy Fireball Mini-Game...");
}

void FlappyFireballGameState::update() {
    // The real game is driven by appMainLoopTick() → updateMiniGame()
    // When the player finishes, mini_games.cpp handles the result and
    // sets g_app.uiState back — no state_manager transition needed here.
}