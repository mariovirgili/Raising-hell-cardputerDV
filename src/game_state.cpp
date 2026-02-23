#include "game_state.h"
#include "state_manager.h"
#include "flappy_fireball_game_state.h"
#include "pause_state.h"

void GameState::enter() {
    Serial.println("Entering GameState...");
    playerPet = Pet();
    lastPetUpdateMs = millis();
    lastFeedMs = millis();
}

void GameState::exit() {
    Serial.println("Exiting GameState...");
}

void GameState::update() {
    const uint32_t now = millis();

    // Update pet stats every 1 second
    if (now - lastPetUpdateMs >= 1000) {
        lastPetUpdateMs = now;
        playerPet.update();
        Serial.print("Pet Status - Health: ");
        Serial.print(playerPet.health);
        Serial.print(", Happiness: ");
        Serial.println(playerPet.happiness);
    }

    // Feed pet every 2 seconds
    if (now - lastFeedMs >= 2000) {
        lastFeedMs = now;
        playerPet.feed(10);
        Serial.println("Pet fed!");
    }
}