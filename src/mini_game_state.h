#pragma once
#include "state.h"
#include <Arduino.h>

class MiniGameState : public State {
public:
    virtual void enter() override {
        Serial.println("Entering MiniGameState...");
        // Initialize game-specific resources (e.g., display, game objects)
    }

    virtual void exit() override {
        Serial.println("Exiting MiniGameState...");
        // Clean up mini-game resources (e.g., stop animations, reset game objects)
    }

    virtual void update() override {
        // Main game loop logic, to be implemented by specific mini-games
        // This can handle user input, game updates, and checking win conditions
    }
};