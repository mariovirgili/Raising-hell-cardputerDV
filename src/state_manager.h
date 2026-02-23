#pragma once
#include "state.h"

class StateManager {
private:
    State* current_state;  // Pointer to the current active state

public:
    StateManager() : current_state(nullptr) {}

    // Set the current state
    void setState(State* new_state) {
        if (current_state) {
            current_state->exit();
            delete current_state;  // Fix: free old state to prevent memory leak
        }
        current_state = new_state;
        if (current_state) {
            current_state->enter();  // Initialize the new state
        }
    }

    // Update the current state
    void update() {
        if (current_state) {
            current_state->update();  // Call the update function of the current state
        }
    }

    // Get the current state
    State* getCurrentState() const {
        return current_state;
    }
};

// Declare the state_manager globally (used throughout the program)
extern StateManager state_manager;