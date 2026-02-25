#pragma once
#include <cstddef>
#include "state.h"

class StateManager {
private:
    State* _current = nullptr;
    State* _next = nullptr;

    bool _current_owned = false; // if true, delete on transition
    bool _next_owned = false;

    void destroyIfOwned_(State*& s, bool& owned) {
        if (!s) return;
        if (owned) {
            delete s;
        }
        s = nullptr;
        owned = false;
    }

    void applyPending_() {
        if (!_next) return;

        if (_current) {
            _current->exit();
        }
        destroyIfOwned_(_current, _current_owned);

        _current = _next;
        _current_owned = _next_owned;

        _next = nullptr;
        _next_owned = false;

        if (_current) {
            _current->enter();
        }
    }

public:
    StateManager() = default;

    // Preferred: pass a pointer to a static/global state (not owned)
    void setState(State* new_state) {
        request(new_state);
    }

    // Back-compat: old code uses heap allocation (owned)
    void setStateOwned(State* new_state) {
        requestOwned(new_state);
    }

    // Existing code compatibility: keep this name too
    void setStateImmediate(State* new_state) {
        request(new_state);
        applyPending_();
    }

    // New API: request a state change (applied at start of update)
    void request(State* new_state) {
        // replace any pending state
        destroyIfOwned_(_next, _next_owned);
        _next = new_state;
        _next_owned = false;
    }

    void requestOwned(State* new_state) {
        destroyIfOwned_(_next, _next_owned);
        _next = new_state;
        _next_owned = true;
    }

    void update() {
        applyPending_();
        if (_current) {
            _current->update();
        }
    }

    State* current() const { return _current; }

    ~StateManager() {
        destroyIfOwned_(_next, _next_owned);
        if (_current) {
            _current->exit();
        }
        destroyIfOwned_(_current, _current_owned);
    }
};

extern StateManager state_manager;