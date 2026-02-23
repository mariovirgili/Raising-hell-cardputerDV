#pragma once
#include "state.h"
#include <Arduino.h>
#include "state_manager_fwd.h"
#include "settings_state.h"  

class GameState;
class PauseState;

class MainMenuState : public State {
public:
    void enter() override;
    void exit() override;
    void update() override;
};