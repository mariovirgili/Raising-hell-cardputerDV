#pragma once

class State {
public:
    virtual void enter() = 0;   // Initialize the state
    virtual void exit() = 0;    // Clean up the state
    virtual void update() = 0;  // Update the state each frame
    virtual ~State() {}
};