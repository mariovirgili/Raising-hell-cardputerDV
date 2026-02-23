#include "resurrection_run_game_state.h"
#include "state_manager.h"
#include "game_state.h"

void ResurrectionRunGameState::enter() {
    Serial.println("Entering Resurrection Run Mini-Game...");
    startResurrectionRun();    // ← launches the real game
}

void ResurrectionRunGameState::exit() {
    Serial.println("Exiting Resurrection Run Mini-Game...");
}

void ResurrectionRunGameState::update() {
    // Driven by appMainLoopTick() → updateMiniGame(), same as Flappy Fireball
}