#include "pause_state.h"
#include "state_manager.h"
#include "game_state.h"
#include "main_menu_state.h"

void PauseState::enter() {
    Serial.println("Entering PauseState...");
}

void PauseState::exit() {
    Serial.println("Exiting PauseState...");
}

void PauseState::update() {
    static int selectedOption = 0;

    if (selectedOption == 0) {
        Serial.println("Resuming game...");
        state_manager.setState(new GameState());
    } else if (selectedOption == 1) {
        Serial.println("Quitting to Main Menu...");
        state_manager.setState(new MainMenuState());
    }
}