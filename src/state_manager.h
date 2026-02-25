#pragma once
#include "state.h"
#include <cstddef>

class StateManager
{
private:
  State *_current = nullptr;
  State *_next = nullptr;

  void applyPending_()
  {
    if (!_next)
      return;

    if (_current)
    {
      _current->exit();
      delete _current;
      _current = nullptr;
    }

    _current = _next;
    _next = nullptr;

    if (_current)
    {
      _current->enter();
    }
  }

public:
  StateManager() = default;

  // Request a state change (will be applied at the start of update()).
  void request(State *new_state)
  {
    // If multiple requests happen before the next update, keep the latest.
    if (_next)
    {
      delete _next;
    }
    _next = new_state;
  }

  // Convenience: immediate change (use sparingly)
  void setStateImmediate(State *new_state)
  {
    request(new_state);
    applyPending_();
  }

  void update()
  {
    applyPending_();
    if (_current)
    {
      _current->update();
    }
  }

  State *current() const { return _current; }

  ~StateManager()
  {
    if (_next)
    {
      delete _next;
      _next = nullptr;
    }
    if (_current)
    {
      _current->exit();
      delete _current;
      _current = nullptr;
    }
  }
  // Back-compat: old code calls setState(...)
  void setState(State *new_state) { request(new_state); }
};

extern StateManager state_manager;