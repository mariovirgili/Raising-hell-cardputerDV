#pragma once
#include "state.h"
#include <Arduino.h>
#include "state_manager_fwd.h"
#include "input.h"

class GameState;
class PauseState;

class MainMenuState : public State {
private:
    int menuOption = 0;
public:
    void enter() override;
    void exit() override;
    void update() override;
};