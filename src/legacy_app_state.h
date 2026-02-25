#pragma once
#include "state.h"

class LegacyAppState : public State {
public:
    void enter() override;
    void exit() override;
    void update() override;
};