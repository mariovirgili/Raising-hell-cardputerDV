#pragma once
#include "state.h"
#include <Arduino.h>
#include "state_manager_fwd.h"

class GameState;
class MainMenuState;

class PauseState : public State {
public:
    void enter() override;
    void exit() override;
    void update() override;
};