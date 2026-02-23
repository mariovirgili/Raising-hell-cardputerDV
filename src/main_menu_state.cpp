#include "main_menu_state.h"
#include "state_manager.h"
#include "game_state.h"
#include "app_state.h"    // for g_app and UIState

void MainMenuState::enter() {
    Serial.println("Entering MainMenuState...");
}

void MainMenuState::exit() {
    Serial.println("Exiting MainMenuState...");
}

void MainMenuState::update() {
    static int menuOption = 0;
    Serial.print("Main menu option selected: ");
    Serial.println(menuOption);

    if (menuOption == 0) {
        Serial.println("Starting game...");
        state_manager.setState(new GameState());
    } else if (menuOption == 1) {
        Serial.println("Opening settings...");
        g_app.uiState = UIState::SETTINGS;  // how the real codebase navigates to settings
    }
}