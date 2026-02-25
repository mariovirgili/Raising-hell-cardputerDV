#include "legacy_app_state.h"
#include "app_loop.h"

void LegacyAppState::enter() {
    // Optional: nothing needed
}

void LegacyAppState::exit() {
    // Optional: nothing needed
}

void LegacyAppState::update() {
    // Run the existing “real” app loop
    appMainLoopTick();
}