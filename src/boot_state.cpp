#include "boot_state.h"
#include "state_manager_fwd.h"
#include "state_manager.h"      // full definition needed to call setState()
#include "main_menu_state.h"    // full definition needed to call new MainMenuState()

bool g_bootSplashActive = true;
uint32_t g_bootCount = 0;

void BootState::enter() {
    g_bootSplashActive = true;
    g_bootCount = 0;
    Serial.println("Entering BootState...");
}

void BootState::exit() {
    g_bootSplashActive = false;
    Serial.println("Exiting BootState...");
}

void BootState::update() {
    if (g_bootSplashActive) {
        g_bootCount++;
        Serial.print("Boot count: ");
        Serial.println(g_bootCount);

        if (g_bootCount > 100) {
            Serial.println("BootState transition...");
            state_manager.setState(new MainMenuState());
        }
    }
}