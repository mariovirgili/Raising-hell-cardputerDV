#pragma once
#include <Arduino.h>
#include "state.h"
#include <stdbool.h>
#include <stdint.h>

extern bool g_bootSplashActive;
extern uint32_t g_bootCount;

class BootState : public State {
public:
    void enter() override;
    void exit() override;
    void update() override;
};