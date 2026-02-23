#include "input.h"

#include <Arduino.h>
#include <ctype.h>
#include "M5Cardputer.h"

#include "currency.h"
#include "display.h"
#include "graphics.h"
#include "app_state.h"

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------
static void readKeyboard(InputState& out);
static void applyGpioButtons(InputState& out);
static void IRAM_ATTR updateEncoderISR();
static bool lastGo = false;
static uint32_t goPressMs = 0;
// -----------------------------------------------------------------------------
// Shared input globals
// -----------------------------------------------------------------------------
static InputState g_last;

// Force-clear flag: request a latch clear on next readInput()
static volatile bool g_forceClear = false;

// If GO is used for screen power, it may also generate Keyboard Enter.
// This flag lets main() consume exactly one Enter->Select mapping.
static volatile bool g_consumeEnterOnce = false;
void inputConsumeEnterOnce() { g_consumeEnterOnce = true; }

// Text capture mode (console/text input)
bool g_textCaptureMode = false;

// Keyboard edge latches
static bool s_settingsKeyLatched = false; // ` or ~
static bool s_enterLatched       = false;
static bool s_mgSelectLatched    = false; // mini-game Enter/G edge latch (separate from UI Enter)
static bool s_delLatched         = false;
static bool s_qLatched           = false; // Q -> menu edge
static bool s_cLatched = false; // '\' -> console edge
static bool s_gLatched = false; // G -> selectOnce edge

// Nav cluster (WASD / HJKL / punctuation) latches — prevent press+release double edges
static bool s_navUpLatched    = false;
static bool s_navDownLatched  = false;
static bool s_navLeftLatched  = false;
static bool s_navRightLatched = false;

static bool s_eLatched = false;
static bool s_sLatched = false;
static bool s_spaceLatched = false; 

static inline bool kbHeldChar(char c) {
  return M5Cardputer.Keyboard.isKeyPressed(c);
}

static inline bool kbHeldKey(uint8_t keycode) {
  return M5Cardputer.Keyboard.isKeyPressed((char)keycode);
}

// Your arrow cluster on Cardputer is usually FN-layer punctuation.
// Common mapping used in Cardputer projects:
//   Up=';'  Down='.'  Left=','  Right='/'
// If yours differs, change these four chars to match your firmware.
static inline bool kbHeldUpArrow()    { return kbHeldChar(';'); }
static inline bool kbHeldDownArrow()  { return kbHeldChar('.'); }
static inline bool kbHeldLeftArrow()  { return kbHeldChar(','); }
static inline bool kbHeldRightArrow() { return kbHeldChar('/'); }

// -----------------------------------------------------------------------------
// OPTIONAL GPIO BUTTONS
// -----------------------------------------------------------------------------
static inline bool readActiveLow(int pin) { return (digitalRead(pin) == LOW); }

static constexpr unsigned long DEBOUNCE_MS = 12;  // 10–15ms feels good on Cardputer buttons

struct DebBtn {
  bool raw = false;
  bool stable = false;
  unsigned long lastChangeMs = 0;
};

#if defined(BTN_LEFT)
static DebBtn dbLeft;
#endif
#if defined(BTN_RIGHT)
static DebBtn dbRight;
#endif
#if defined(BTN_UP)
static DebBtn dbUp;
#endif
#if defined(BTN_DOWN)
static DebBtn dbDown;
#endif
#if defined(BTN_SELECT)
static DebBtn dbSel;
#endif
#if defined(BTN_MENU)
static DebBtn dbMenu;
#endif


// -----------------------------------------------------------------------------
// OPTIONAL ENCODER (kept behind defines; safe if unused)
// -----------------------------------------------------------------------------
volatile int g_detentDelta = 0;
static volatile uint8_t g_lastEncState = 0;
static volatile int8_t  g_quadCount    = 0;

static const int8_t QEM[16] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

static void IRAM_ATTR updateEncoderISR() {
#if defined(ENC_A) && defined(ENC_B)
  uint8_t a = (uint8_t)digitalRead(ENC_A);
  uint8_t b = (uint8_t)digitalRead(ENC_B);
  uint8_t state = (uint8_t)((a << 1) | b);

  uint8_t idx = (uint8_t)((g_lastEncState << 2) | state);
  int8_t step = QEM[idx];

  if (step != 0) {
    g_quadCount += step;
    if (g_quadCount >= 4) { g_detentDelta++; g_quadCount = 0; }
    else if (g_quadCount <= -4) { g_detentDelta--; g_quadCount = 0; }
  }

  g_lastEncState = state;
#endif
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void inputForceClear() { g_forceClear = true; }

void inputSetTextCapture(bool on) {
  g_textCaptureMode = on;

  // Clear latches immediately so we don't carry UI edges into text or vice versa.
  clearInputLatch();

  // Also request a one-tick deferred clear so "cleared while held" doesn't
  // starve edges after mode switches.
  inputForceClear();
}

void inputInit() {
  // Optional GPIO buttons
 const unsigned long now = millis();

#if defined(BTN_LEFT)
  pinMode(BTN_LEFT, INPUT_PULLUP);
  dbLeft.raw = dbLeft.stable = readActiveLow(BTN_LEFT);
  dbLeft.lastChangeMs = now;
#endif
#if defined(BTN_RIGHT)
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  dbRight.raw = dbRight.stable = readActiveLow(BTN_RIGHT);
  dbRight.lastChangeMs = now;
#endif
#if defined(BTN_UP)
  pinMode(BTN_UP, INPUT_PULLUP);
  dbUp.raw = dbUp.stable = readActiveLow(BTN_UP);
  dbUp.lastChangeMs = now;
#endif
#if defined(BTN_DOWN)
  pinMode(BTN_DOWN, INPUT_PULLUP);
  dbDown.raw = dbDown.stable = readActiveLow(BTN_DOWN);
  dbDown.lastChangeMs = now;
#endif
#if defined(BTN_SELECT)
  pinMode(BTN_SELECT, INPUT_PULLUP);
  dbSel.raw = dbSel.stable = readActiveLow(BTN_SELECT);
  dbSel.lastChangeMs = now;
#endif
#if defined(BTN_MENU)
  pinMode(BTN_MENU, INPUT_PULLUP);
  dbMenu.raw = dbMenu.stable = readActiveLow(BTN_MENU);
  dbMenu.lastChangeMs = now;
#endif

  // Optional encoder (only if you define ENC_A/ENC_B)
#if defined(ENC_A) && defined(ENC_B)
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  g_lastEncState = (uint8_t)((digitalRead(ENC_A) << 1) | digitalRead(ENC_B));
  attachInterrupt(digitalPinToInterrupt(ENC_A), updateEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), updateEncoderISR, CHANGE);
#endif

  g_detentDelta = 0;
  g_quadCount   = 0;

  s_settingsKeyLatched = false;
  s_enterLatched       = false;
  s_mgSelectLatched    = false;
  s_delLatched         = false;
  s_qLatched           = false;
    s_cLatched           = false;
  s_gLatched           = false;
  s_navUpLatched       = false;
  s_navDownLatched     = false;
  s_navLeftLatched     = false;
  s_navRightLatched    = false;
}

void clearInputLatch() {
  // Reset keyboard latches.
  // IMPORTANT: preserve latches for keys that are currently HELD.
  // Otherwise, calling clearInputLatch() while ENTER is held will synthesize a
  // fresh "Enter press" next tick -> selectOnce -> wakes pet on PET_SLEEPING.
  auto st = M5Cardputer.Keyboard.keysState();

  s_settingsKeyLatched = false;     // ok to reset; it's edge-only for ` / ~
  s_enterLatched       = st.enter;  // preserve held state
  s_delLatched         = st.del;    // preserve held state

  s_qLatched           = false;
  s_cLatched           = false;
  s_gLatched           = false;

  s_navUpLatched       = false;
  s_navDownLatched     = false;
  s_navLeftLatched     = false;
  s_navRightLatched    = false;
s_mgSelectLatched = false;

  // Reset edge tracking for GPIO so next tick doesn't generate "Once"
 const unsigned long now = millis();

#if defined(BTN_LEFT)
  dbLeft.raw = dbLeft.stable = readActiveLow(BTN_LEFT);
  dbLeft.lastChangeMs = now;
#endif
#if defined(BTN_RIGHT)
  dbRight.raw = dbRight.stable = readActiveLow(BTN_RIGHT);
  dbRight.lastChangeMs = now;
#endif
#if defined(BTN_UP)
  dbUp.raw = dbUp.stable = readActiveLow(BTN_UP);
  dbUp.lastChangeMs = now;
#endif
#if defined(BTN_DOWN)
  dbDown.raw = dbDown.stable = readActiveLow(BTN_DOWN);
  dbDown.lastChangeMs = now;
#endif
#if defined(BTN_SELECT)
  dbSel.raw = dbSel.stable = readActiveLow(BTN_SELECT);
  dbSel.lastChangeMs = now;
#endif
#if defined(BTN_MENU)
  dbMenu.raw = dbMenu.stable = readActiveLow(BTN_MENU);
  dbMenu.lastChangeMs = now;
#endif


  // Reset encoder movement
  g_detentDelta = 0;
  g_quadCount   = 0;
}

// -----------------------------------------------------------------------------
// Main input read (matches input.h API)
// -----------------------------------------------------------------------------
void readInput(InputState& out) {
  if (g_forceClear) {
    clearInputLatch();
    g_forceClear = false;
  }

  out.clearEdges();

  // Keyboard first (fills kb queue and sets UI pulses depending on mode)
  readKeyboard(out);

  // Only apply physical buttons / encoder in UI mode
  if (!g_textCaptureMode) {
    applyGpioButtons(out);

    // Optional encoder -> encoderDelta (if ever enabled)
    int d = 0;
    noInterrupts();
    d = g_detentDelta;
    g_detentDelta = 0;
    interrupts();
    out.encoderDelta = d;
  }

  g_last = out;
}

// Wrapper for old code: InputState in = readInput();
InputState readInput() {
  InputState out;
  readInput(out);
  return out;
}

// -----------------------------------------------------------------------------
// GPIO buttons (edge pulses)
// -----------------------------------------------------------------------------
static void applyGpioButtons(InputState& out) {
  const unsigned long now = millis();

  auto updateBtn = [&](int pin, DebBtn& b, bool& onceFlag) {
    const bool cur = readActiveLow(pin);

    // Track raw changes
    if (cur != b.raw) {
      b.raw = cur;
      b.lastChangeMs = now;
    }

    // When raw has been stable long enough, commit to stable state
    if ((now - b.lastChangeMs) >= DEBOUNCE_MS && b.stable != b.raw) {
      b.stable = b.raw;

      // Fire edge on press only
      if (b.stable) onceFlag = true;
    }
  };

  // --------------------------------------------------------------------------
  // 1) Compute normal UI once flags from GPIO (debounced)
  // --------------------------------------------------------------------------
#if defined(BTN_LEFT)
  updateBtn(BTN_LEFT, dbLeft, out.leftOnce);
#endif
#if defined(BTN_RIGHT)
  updateBtn(BTN_RIGHT, dbRight, out.rightOnce);
#endif
#if defined(BTN_UP)
  updateBtn(BTN_UP, dbUp, out.upOnce);
#endif
#if defined(BTN_DOWN)
  updateBtn(BTN_DOWN, dbDown, out.downOnce);
#endif
#if defined(BTN_SELECT)
  updateBtn(BTN_SELECT, dbSel, out.selectOnce);
#endif
#if defined(BTN_MENU)
  updateBtn(BTN_MENU, dbMenu, out.menuOnce);
#endif

  // --------------------------------------------------------------------------
  // 2) Compute normal UI held flags (debounced stable state)
  // --------------------------------------------------------------------------
#if defined(BTN_SELECT)
  out.selectHeld = dbSel.stable;
#endif
#if defined(BTN_MENU)
  out.menuHeld = dbMenu.stable;
#endif
#if defined(BTN_UP)
  out.upHeld = dbUp.stable;
#endif
#if defined(BTN_DOWN)
  out.downHeld = dbDown.stable;
#endif
#if defined(BTN_LEFT)
  out.leftHeld = dbLeft.stable;
#endif
#if defined(BTN_RIGHT)
  out.rightHeld = dbRight.stable;
#endif

  // --------------------------------------------------------------------------
  // 3) Mini-game mapping + lockout
  //    - Fold GPIO controls into mg* controls
  //    - Prevent GPIO MENU/SELECT from acting as UI hotkeys while in a mini-game
  // --------------------------------------------------------------------------
const bool inMiniGame = (g_app.uiState == UIState::MINI_GAME);

  if (inMiniGame) {
    // Fold directional edges + held
    out.mgLeftOnce   |= out.leftOnce;
    out.mgRightOnce  |= out.rightOnce;
    out.mgUpOnce     |= out.upOnce;
    out.mgDownOnce   |= out.downOnce;

    out.mgLeftHeld   |= out.leftHeld;
    out.mgRightHeld  |= out.rightHeld;
    out.mgUpHeld     |= out.upHeld;
    out.mgDownHeld   |= out.downHeld;

    // Fold SELECT as mini-game "action"
    out.mgSelectOnce |= out.selectOnce;
    out.mgSelectHeld |= out.selectHeld;

    // IMPORTANT:
    // Lock out the UI-layer meanings so physical buttons can't trigger
    // tab jumps / menu backing / UI sounds / force quits while in a mini-game.
    out.leftOnce = false;
    out.rightOnce = false;
    out.upOnce = false;
    out.downOnce = false;
    out.selectOnce = false;
    out.menuOnce = false;

    out.leftHeld = false;
    out.rightHeld = false;
    out.upHeld = false;
    out.downHeld = false;
    out.selectHeld = false;
    out.menuHeld = false;
  }
}

// -----------------------------------------------------------------------------
// Keyboard read + mapping
// -----------------------------------------------------------------------------
static void readKeyboard(InputState& out) {
  // Note: requires M5Cardputer.update() in the main loop
  const bool changed = M5Cardputer.Keyboard.isChange();

  // Always sample current state so HELD-level signals work even when keys don't "change".
  auto st = M5Cardputer.Keyboard.keysState();
  out.kbHeldCount = (uint8_t)M5Cardputer.Keyboard.isPressed();

  const bool inMiniGameUi = (g_app.uiState == UIState::MINI_GAME);

  // --------------------------------------------------------------------------
  // MINI-GAME mapped HELD flags (always computed)
  // These are independent of st.word so mini-games have stable controls.
  // --------------------------------------------------------------------------
  const bool heldUp    = kbHeldUpArrow();
  const bool heldDown  = kbHeldDownArrow();
  const bool heldLeft  = kbHeldLeftArrow();
  const bool heldRight = kbHeldRightArrow();

  const bool uiArrowsAllowed = (!g_textCaptureMode && !inMiniGameUi);

  const bool uiUpHeldPunc    = uiArrowsAllowed && heldUp;    // ';'
  const bool uiDownHeldPunc  = uiArrowsAllowed && heldDown;  // '.'
  const bool uiLeftHeldPunc  = uiArrowsAllowed && heldLeft;  // ','
  const bool uiRightHeldPunc = uiArrowsAllowed && heldRight; // '/'

  const bool heldE = kbHeldChar('e') || kbHeldChar('E');
  const bool heldA = kbHeldChar('a') || kbHeldChar('A');
  const bool heldS = kbHeldChar('s') || kbHeldChar('S');
  const bool heldD = kbHeldChar('d') || kbHeldChar('D');

  const bool heldO = kbHeldChar('o') || kbHeldChar('O');
  const bool heldJ = kbHeldChar('j') || kbHeldChar('J');
  const bool heldK = kbHeldChar('k') || kbHeldChar('K');
  const bool heldL = kbHeldChar('l') || kbHeldChar('L');

  const bool heldEnter = st.enter;
  const bool heldG     = kbHeldChar('g') || kbHeldChar('G');
  const bool heldSpace = kbHeldChar(' ');
  const bool heldTilde = kbHeldChar('`') || kbHeldChar('~'); // your ESC key

  const bool mgUpHeld     = (heldUp || heldE || heldO);
  const bool mgDownHeld   = (heldDown || heldS || heldK);
  const bool mgLeftHeld   = (heldLeft || heldA || heldJ);
  const bool mgRightHeld  = (heldRight || heldD || heldL);
  const bool mgSelectHeld = (heldEnter || heldG);

  out.mgUpHeld     = mgUpHeld;
  out.mgDownHeld   = mgDownHeld;
  out.mgLeftHeld   = mgLeftHeld;
  out.mgRightHeld  = mgRightHeld;
  out.mgSelectHeld = mgSelectHeld;
  out.mgSpaceHeld  = heldSpace;

  // Normal UI HELD flags from keyboard (only when not in text capture, and not in mini-game)
  // (Mini-games should not bleed into UI beeps/actions.)
  if (!g_textCaptureMode && !inMiniGameUi) {
    out.selectHeld = st.enter;
  }

  // If nothing changed, don't generate edges / queue events this tick.
  if (!changed) return;

  out.kbChanged = true;

  // --------------------------------------------------------------------------
  // UI punctuation arrows (Cardputer): Up=';' Down='.' Left=',' Right='/'
  // Generate UI nav edges from HELD state so it works even if st.word doesn't
  // contain punctuation reliably.
  //
  // NOTE: This is self-contained (doesn't depend on acceptNav/s_nav*Ms).
  // It uses a small cooldown to match the feel of your other nav gating.
  // --------------------------------------------------------------------------
  if (!inMiniGameUi && !g_textCaptureMode) {
    const bool pUp    = kbHeldUpArrow();    // ';'
    const bool pDown  = kbHeldDownArrow();  // '.'
    const bool pLeft  = kbHeldLeftArrow();  // ','
    const bool pRight = kbHeldRightArrow(); // '/'

    static bool     s_pUpLatched    = false;
    static bool     s_pDownLatched  = false;
    static bool     s_pLeftLatched  = false;
    static bool     s_pRightLatched = false;
    static uint32_t s_pUpMs         = 0;
    static uint32_t s_pDownMs       = 0;
    static uint32_t s_pLeftMs       = 0;
    static uint32_t s_pRightMs      = 0;

    const uint32_t kPuncCooldownMs = 120;
    const uint32_t t = millis();

    auto acceptP = [&](uint32_t& lastMs) -> bool {
      if ((t - lastMs) < kPuncCooldownMs) return false;
      lastMs = t;
      return true;
    };

    if (pUp && !s_pUpLatched && acceptP(s_pUpMs)) {
      out.upOnce = true;
      s_pUpLatched = true;
    }
    if (!pUp) s_pUpLatched = false;

    if (pDown && !s_pDownLatched && acceptP(s_pDownMs)) {
      out.downOnce = true;
      s_pDownLatched = true;
    }
    if (!pDown) s_pDownLatched = false;

    if (pLeft && !s_pLeftLatched && acceptP(s_pLeftMs)) {
      out.leftOnce = true;
      s_pLeftLatched = true;
    }
    if (!pLeft) s_pLeftLatched = false;

    if (pRight && !s_pRightLatched && acceptP(s_pRightMs)) {
      out.rightOnce = true;
      s_pRightLatched = true;
    }
    if (!pRight) s_pRightLatched = false;
  }

  // ---------------------------------------------------------------------------
  // Nav de-dupe / cooldown (preserve your older behavior)
  // ---------------------------------------------------------------------------
  const uint32_t now = millis();

  static uint32_t s_navUpMs    = 0;
  static uint32_t s_navDownMs  = 0;
  static uint32_t s_navLeftMs  = 0;
  static uint32_t s_navRightMs = 0;
  static uint32_t s_navSelMs   = 0;
  static uint32_t s_navMenuMs  = 0;
  static uint32_t s_navConMs   = 0;

  const uint32_t kNavCooldownMs = 120;

  auto acceptNav = [&](uint32_t& lastMs) -> bool {
    if ((now - lastMs) < kNavCooldownMs) return false;
    lastMs = now;
    return true;
  };

  // Release edge latches when idle
  if (out.kbHeldCount == 0) {
    s_qLatched = false;
    s_settingsKeyLatched = false;
    s_cLatched = false;
    s_gLatched = false;
    s_spaceLatched = false;
  }

  // Track whether keys are present in st.word this tick (for latch release)
  bool sawSettingsKeyThisTick = false;

  bool sawUpThisTick = false;
  bool sawDownThisTick = false;
  bool sawLeftThisTick = false;
  bool sawRightThisTick = false;
  bool sawSpaceThisTick = false;

  // --------------------------------------------------------------------------
  // ESC / Quit (` or ~)
  // - Always generate escOnce (legacy)
  // - In mini-games also generate mgQuitOnce
  // --------------------------------------------------------------------------
  if (heldTilde && !s_settingsKeyLatched) {
    out.escOnce = true;
    if (inMiniGameUi) out.mgQuitOnce = true;
    s_settingsKeyLatched = true;
  }
  if (!heldTilde) s_settingsKeyLatched = false;

  // --------------------------------------------------------------------------
  // Generate mg* "Once" edges from held states (independent of word stream)
  // --------------------------------------------------------------------------
  if (mgUpHeld && !s_navUpLatched) {
    out.mgUpOnce = true;
    s_navUpLatched = true;
  }
  if (!mgUpHeld) s_navUpLatched = false;

  if (mgDownHeld && !s_navDownLatched) {
    out.mgDownOnce = true;
    s_navDownLatched = true;
  }
  if (!mgDownHeld) s_navDownLatched = false;

  if (mgLeftHeld && !s_navLeftLatched) {
    out.mgLeftOnce = true;
    s_navLeftLatched = true;
  }
  if (!mgLeftHeld) s_navLeftLatched = false;

  if (mgRightHeld && !s_navRightLatched) {
    out.mgRightOnce = true;
    s_navRightLatched = true;
  }
  if (!mgRightHeld) s_navRightLatched = false;

if (mgSelectHeld && !s_mgSelectLatched) {
  out.mgSelectOnce = true;
  s_mgSelectLatched = true;
}
if (!mgSelectHeld) s_mgSelectLatched = false;

  if (heldSpace && !s_spaceLatched) {
    out.mgSpaceOnce = true;
    s_spaceLatched = true;
  }
  if (!heldSpace) s_spaceLatched = false;

  // --------------------------------------------------------------------------
  // If we're in a mini-game: HARD LOCKOUT of UI hotkeys and word processing.
  // Also sync latches so suppressed keys don't "fire" after leaving the mini-game.
  // --------------------------------------------------------------------------
  if (inMiniGameUi) {
    // Suppress UI actions entirely
    out.tabJump = 255;
    out.consoleOnce = false;
    out.controlsOnce = false;
    out.hotSettings = false;
    out.menuOnce = false;
    out.selectOnce = false;

    // Keep these latches synced so keys pressed during mini-game won't fire later
    s_qLatched = kbHeldChar('q') || kbHeldChar('Q');
    s_cLatched = kbHeldChar('\\');
    s_gLatched = heldG;

    // Don't push any chars into kb queue while in mini-games
    out.kbQHead = out.kbQTail = 0;
    return;
  }

  // --------------------------------------------------------------------------
  // NOT in mini-game: restore the legacy UI mappings from st.word.
  // This drives tab hotkeys and menu navigation.
  //
  // IMPORTANT:
  //  - Do NOT rely on st.word for punctuation arrows (; . , /). Those are now
  //    handled via held-state (kbHeldUpArrow/Down/Left/Right) so they're reliable.
  //  - st.word remains the source for letter hotkeys (tabs, Q, \, G, I, etc.)
  // --------------------------------------------------------------------------
  for (auto c : st.word) {
    if (!c) continue;

    // Some cores put HID usage IDs into st.word for special keys.
    if ((uint8_t)c == 0x28) { out.kbPush((uint8_t)'\n'); continue; }  // Enter
    if ((uint8_t)c == 0x2A) { out.kbPush((uint8_t)'\b'); continue; }  // Backspace

    // ` or ~ already handled via heldTilde (but keep sawSettingsKeyThisTick for safety)
    if (c == '`' || c == '~') {
      sawSettingsKeyThisTick = true;
      continue;
    }

    const char lc = (char)tolower((unsigned char)c);

    // UI nav mapping only when NOT in text capture mode
    if (!g_textCaptureMode) {
      // Q -> menu edge
      if (lc == 'q') {
        if (!s_qLatched && acceptNav(s_navMenuMs)) out.menuOnce = true;
        s_qLatched = true;
        continue;
      }

      // '\' -> open console (edge-triggered; NOT in mini-games handled above)
      if (c == '\\') {
        if (!s_cLatched && acceptNav(s_navConMs)) {
          out.consoleOnce = true;
          s_cLatched = true;
        }
        continue; // do NOT push into kb queue
      }

      // G -> selectOnce (Enter equivalent), UI mode only
      if (lc == 'g') {
        if (!s_gLatched && acceptNav(s_navSelMs)) {
          out.selectOnce = true;
          s_gLatched = true;
        }
        continue; // do NOT push into kb queue
      }

      // I -> Controls help overlay hotkey (UI mode only)
      if (lc == 'i') {
        out.controlsOnce = true;
        continue; // do NOT push into kb queue
      }

      // Bottom row -> direct tab jumps (do NOT push into kb queue)
      // z pet, x stats, c feed, v play, b sleep, n inventory, m shop
      switch (lc) {
        case 'z': out.tabJump = 0; continue; // Tab::TAB_PET
        case 'x': out.tabJump = 1; continue; // Tab::TAB_STATS
        case 'c': out.tabJump = 2; continue; // Tab::TAB_FEED
        case 'v': out.tabJump = 3; continue; // Tab::TAB_PLAY
        case 'b': out.tabJump = 4; continue; // Tab::TAB_SLEEP
        case 'n': out.tabJump = 5; continue; // Tab::TAB_INV
        case 'm': out.tabJump = 6; continue; // Tab::TAB_SHOP
        default: break;
      }

      // Existing nav cluster (latched + cooldown)
      // NOTE: punctuation arrows (; . , /) are handled via held-state elsewhere.
      if (lc == 'e' || lc == 'w' || lc == 'o') {
        sawUpThisTick = true;
        if (!s_navUpLatched && acceptNav(s_navUpMs)) out.upOnce = true;
        s_navUpLatched = true;
        continue;
      }
      if (lc == 'a' || lc == 'j') {
        sawLeftThisTick = true;
        if (!s_navLeftLatched && acceptNav(s_navLeftMs)) out.leftOnce = true;
        s_navLeftLatched = true;
        continue;
      }
      if (lc == 's' || lc == 'k') {
        sawDownThisTick = true;
        if (!s_navDownLatched && acceptNav(s_navDownMs)) out.downOnce = true;
        s_navDownLatched = true;
        continue;
      }
      if (lc == 'd' || lc == 'l') {
        sawRightThisTick = true;
        if (!s_navRightLatched && acceptNav(s_navRightMs)) out.rightOnce = true;
        s_navRightLatched = true;
        continue;
      }
    }

    // Otherwise: normal printable char goes to text queue
    out.kbPush((uint8_t)c);
  }

  // Release nav latches when the key is no longer present in st.word.
  // This makes nav edges fire on press only, not on release.
  if (!sawUpThisTick) s_navUpLatched = false;
  if (!sawDownThisTick) s_navDownLatched = false;
  if (!sawLeftThisTick) s_navLeftLatched = false;
  if (!sawRightThisTick) s_navRightLatched = false;

  // Allow another escOnce when not holding ` or ~
  if (out.kbHeldCount == 0) s_settingsKeyLatched = false;
  else if (!sawSettingsKeyThisTick) s_settingsKeyLatched = false;

  // Backspace/Delete (edge) + optional UI menu mapping
  if (st.del) {
    if (!s_delLatched) out.kbPush((uint8_t)'\b');
    if (!s_delLatched && !g_textCaptureMode) {
      if (acceptNav(s_navMenuMs)) out.menuOnce = true;
    }
    s_delLatched = true;
  } else {
    s_delLatched = false;
  }

  // Enter (edge) + optional UI select mapping
  if (st.enter) {
    if (!s_enterLatched) out.kbPush((uint8_t)'\n');

    if (!s_enterLatched && !g_textCaptureMode) {
      if (g_consumeEnterOnce) g_consumeEnterOnce = false;
      else {
        if (acceptNav(s_navSelMs)) out.selectOnce = true;
      }
    }

    s_enterLatched = true;
  } else {
    s_enterLatched = false;
  }

  if (st.fn)    out.kbPush((uint8_t)KEY_FN);
  if (st.shift) out.kbPush((uint8_t)RH_KEY_SHIFT);
}
