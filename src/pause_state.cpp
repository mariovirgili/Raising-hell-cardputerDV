#include "pause_state.h"
#include "state_manager.h"
#include "game_state.h"
#include "main_menu_state.h"
#include "input.h"
#include "sound.h"

void PauseState::enter() {
    Serial.println("Entering PauseState...");
    selectedOption = 0;
}

void PauseState::exit() {
    Serial.println("Exiting PauseState...");
}

void PauseState::update() {
    InputState input = readInput();

    // Navigate options
    if (input.upOnce || input.leftOnce) {
        if (selectedOption > 0) {
            selectedOption--;
            playBeep();
        }
    }
    if (input.downOnce || input.rightOnce) {
        if (selectedOption < 1) {
            selectedOption++;
            playBeep();
        }
    }

    // Confirm selection
    if (input.selectOnce || input.encoderPressOnce) {
        if (selectedOption == 0) {
            Serial.println("Resuming game...");
            state_manager.setStateOwned(new GameState());        } else {
            Serial.println("Quitting to Main Menu...");
            state_manager.setStateOwned(new MainMenuState());        }
        return;
    }
}