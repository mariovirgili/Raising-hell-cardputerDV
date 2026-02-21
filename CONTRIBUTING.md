# Contributing to Raising Hell

Thanks for your interest in contributing.

## Development Guidelines

- Do not reintroduce global state.
- Avoid including legacy headers in new code.
- Maintain clear separation between:
  - Core
  - UI
  - Gameplay
  - Hardware/platform

## Pull Request Guidelines

- Must compile for M5Cardputer
- Must not break SD asset loading
- Keep includes minimal
- Document architectural changes

## Style

- Use clear function boundaries.
- Prefer explicit state references over globals.
- Keep platform-specific logic isolated.

---

If you're unsure about architecture direction, open an issue first.