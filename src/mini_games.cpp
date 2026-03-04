// ---------------------------------------------------------------------------
// Mini-game implementation toggle
//
// Default: implementation lives in mini_games.cpp.
// If you ever move implementation back to the pause-menu module, set
// RH_MINIGAMES_IMPL_IN_PAUSE_MENU=1 (e.g. via build_flags.h) and this file
// becomes an intentional stub to avoid duplicate symbols.
// ---------------------------------------------------------------------------

#ifndef RH_MINIGAMES_IMPL_IN_PAUSE_MENU
#define RH_MINIGAMES_IMPL_IN_PAUSE_MENU 0
#endif

#if RH_MINIGAMES_IMPL_IN_PAUSE_MENU

#include "mini_games.h"
// Intentionally empty (implementation moved elsewhere).

#else

#include <stdint.h>

#include "mini_games.h"

#include <Arduino.h>

#include "app_state.h" // g_app
#include "display.h"
#include "graphics.h"   // spr, screenW/screenH, invalidateBackgroundCache()
#include "sound.h"      // soundFlap/soundConfirm/soundError/playBeep
#include "ui_defs.h"    // UIState
#include "ui_runtime.h" // requestUIRedraw()

#include "currency.h"
#include "inventory.h"     // ItemType
#include "mg_pause_core.h" // mgPause* + MGPAUSE_* constants
#include "mg_pause_menu.h"
#include "mini_game_return_ui.h" // miniGameSetReturnUi / miniGameGetReturnUiOrDefault / miniGameClearReturnUi
#include "pet.h"
#include "save_manager.h"
#include "ui_actions.h"
#include "input.h"

// -----------------------------------------------------------------------------
// Mini-game input helpers / shared state
// -----------------------------------------------------------------------------
static bool s_mgExiting = false;

static bool s_acceptArmed = false;
static uint32_t s_gameOverMs = 0;

static bool s_showReward = false;
static char s_rewardMsg[64] = {0};

// Forward decls used by multiple mini-games / defined later in this file
static int  rollMiniGameInfReward();
static void exitMiniGameToReturnUi(bool beginLockout = true);
static void mgApplyResultAndShowReward(bool won);
static bool tryAwardWinItem_1in4(ItemType *outType);
static void mgSyncGameTimebases(uint32_t now);
static inline bool mgInputLockedOut();

// Crossy Road
void startCrossyRoad();
void updateCrossyRoad(const InputState &input);
void drawCrossyRoad();

// IMPORTANT:
// We synthesize "Enter once" from selectHeld edge. Do NOT reset this to false
// during transitions, or holding Enter will instantly auto-dismiss the next screen.
static bool s_prevSelectHeld = false;

static bool miniGameEnterOnce(const InputState &input)
{
  const bool held = input.mgSelectHeld;

  // During launch/transition lockout, do NOT generate an enterOnce edge.
  // Still track held state so we don't synthesize a fake edge when lockout ends.
  if (mgInputLockedOut())
  {
    s_prevSelectHeld = held;
    return false;
  }

  const bool enterOnce = (held && !s_prevSelectHeld);
  s_prevSelectHeld = held;
  return enterOnce || input.mgSelectOnce;
}

static const char* mgItemName(ItemType t)
{
  // Preferred: use inventory’s label function (old reference behavior).
  // If this doesn't compile, see fallback note below.
  const char* nm = g_app.inventory.getItemLabelForType(t);
  if (nm && nm[0]) return nm;

  // Fallback: keep something readable even if labels aren’t available.
  switch (t)
  {
    case ITEM_SOUL_FOOD:     return "SOUL FOOD";
    case ITEM_CURSED_RELIC:  return "CURSED RELIC";
    case ITEM_DEMON_BONE:    return "DEMON BONE";
    case ITEM_RITUAL_CHALK:  return "RITUAL CHALK";
    default:                return "ITEM";
  }
}

// -----------------------------------------------------------------------------
// Mini-game global state
// -----------------------------------------------------------------------------
MiniGame currentMiniGame = MiniGame::NONE;
bool playerWon = false;

// Simple mini-game state
static bool s_resultShown = false;

static uint32_t s_mgInputLockoutUntilMs = 0;

static inline void mgBeginInputLockout(uint32_t ms) { s_mgInputLockoutUntilMs = millis() + ms; }

static inline bool mgInputLockedOut() { return (int32_t)(millis() - s_mgInputLockoutUntilMs) < 0; }

static constexpr uint32_t kSurviveWinMs = 15000; // or whatever your old value was

static void mgArmAccept(uint32_t now, uint32_t delayMs = 180)
{
  s_acceptArmed = false;
  s_gameOverMs = now + delayMs;
}

static bool mgAcceptArmedNow(uint32_t now)
{
  if (!s_acceptArmed && (int32_t)(now - s_gameOverMs) >= 0)
    s_acceptArmed = true;
  return s_acceptArmed && !mgInputLockedOut();
}

// -----------------------------------------------------------------------------
// FLAPPY FIREBALL (Flappy Bird clone)
// -----------------------------------------------------------------------------

static bool s_flappyInited = false;
static bool s_flappyPlaying = false;
static bool s_flappyCrashed = false;

static uint32_t s_flappyStartMs = 0;

// Survive timer
static const uint32_t s_flappyWinMs = kSurviveWinMs;

static int s_fbX = 0;
static int s_fbY = 0;
static int s_fbVY = 0;
static uint32_t s_lastStepMs = 0;

struct FlappyPipe
{
  int x;
  int gapY; // center of gap
  bool passed;
};

static FlappyPipe s_pipes[3];

static int flappyRandGapY(int h)
{
  const int gapH = 64;
  const int margin = 14;
  const int lo = margin + gapH / 2;
  const int hi = (h - 1) - margin - gapH / 2;
  if (hi <= lo)
    return h / 2;
  return lo + (int)random((long)(hi - lo + 1));
}

static void flappyResetWorld(int w, int h)
{
  s_flappyStartMs = millis();
  s_flappyPlaying = true;
  s_flappyCrashed = false;

  s_fbX = 52;
  s_fbY = h / 2;
  s_fbVY = 0;

  const int spacing = 140;
  const int startX = w + 30;

  for (int i = 0; i < 3; ++i)
  {
    s_pipes[i].x = startX + i * spacing;
    s_pipes[i].gapY = flappyRandGapY(h);
    s_pipes[i].passed = false;
  }
}

void startFlappyFireball()
{
  mgPauseReset();

  // Ensure keyboard nav works (ENTER/W) even if console/text mode was left on.
  inputSetTextCapture(false);

  g_app.inMiniGame = true;
  g_app.gameOver = false;
  playerWon = false;
  s_resultShown = false;

  s_showReward = false;
  s_rewardMsg[0] = 0;

  s_acceptArmed = false;
  s_gameOverMs = 0;

  s_prevSelectHeld = false;

  currentMiniGame = MiniGame::FLAPPY_FIREBALL;

  // Never allow "return UI" to be MINI_GAME / MG_PAUSE (causes exit->bounce/lock).
  UIState retUi = g_app.uiState;
  if (retUi == UIState::MINI_GAME || retUi == UIState::MG_PAUSE)
    retUi = UIState::PET_SCREEN;

  miniGameSetReturnUi(retUi);
  uiActionEnterState(UIState::MINI_GAME, g_app.currentTab, false);

  s_flappyInited = false;
  s_lastStepMs = millis();

  invalidateBackgroundCache();
  s_flappyStartMs = millis();
  requestUIRedraw();
  clearInputLatch();
  mgBeginInputLockout(220);
}

static bool flappyCollides(int fbX, int fbY, int r, const FlappyPipe &p, int w, int h)
{
  const int pipeW = 26;
  const int gapH = 64;

  const int kPipeInsetPx = 2;
  const int kGapBonusPx = 4;
  const int kScreenForgive = 2;

  if (fbY - r < -kScreenForgive)
    return true;
  if (fbY + r >= h + kScreenForgive)
    return true;

  int pipeL = p.x + kPipeInsetPx;
  int pipeR = p.x + pipeW - kPipeInsetPx;

  if (fbX + r < pipeL)
    return false;
  if (fbX - r > pipeR)
    return false;

  int gapTop = (p.gapY - gapH / 2) - kGapBonusPx;
  int gapBot = (p.gapY + gapH / 2) + kGapBonusPx;

  if (gapTop < 0)
    gapTop = 0;
  if (gapBot > h)
    gapBot = h;

  if (fbY - r < gapTop)
    return true;
  if (fbY + r > gapBot)
    return true;

  return false;
}

static void flappyStep(int w, int h, bool flap)
{
  const int pipeW = 26;
  const int speedX = 1;
  const int fbR = 4;

  const int gravity = 1;
  const int flapVY = -3;

  static uint8_t s_gravCounter = 0;

  if (flap)
  {
    s_fbVY = flapVY;
    soundFlap();
  }

  s_gravCounter++;
  if (s_gravCounter >= 3)
  {
    s_gravCounter = 0;
    s_fbVY += gravity;
  }

  if (s_fbVY > 4)
    s_fbVY = 4;
  if (s_fbVY < -6)
    s_fbVY = -6;

  s_fbY += s_fbVY;

  int rightMost = s_pipes[0].x;
  for (int i = 1; i < 3; ++i)
    if (s_pipes[i].x > rightMost)
      rightMost = s_pipes[i].x;

  for (int i = 0; i < 3; ++i)
  {
    s_pipes[i].x -= speedX;

    if (s_pipes[i].x < -pipeW)
    {
      s_pipes[i].x = rightMost + 140;
      s_pipes[i].gapY = flappyRandGapY(h);
      s_pipes[i].passed = false;
      rightMost = s_pipes[i].x;
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    if (flappyCollides(s_fbX, s_fbY, fbR, s_pipes[i], w, h))
    {
      playerWon = false;
      g_app.gameOver = true;
      requestUIRedraw();
      s_resultShown = true;
      s_flappyPlaying = false;
      soundError();
      return;
    }
  }
}

static inline uint32_t flappyAliveMsNow(uint32_t now)
{
  uint32_t elapsed = now - s_flappyStartMs;

  const uint32_t pausedAccum = mgPauseAccumMs();
  if (elapsed > pausedAccum)
    elapsed -= pausedAccum;
  else
    elapsed = 0;

  if (mgPauseIsPaused() && mgPauseStartMs() != 0)
  {
    uint32_t pausedSoFar = now - mgPauseStartMs();
    if (elapsed > pausedSoFar)
      elapsed -= pausedSoFar;
    else
      elapsed = 0;
  }

  return elapsed;
}

void updateFlappyFireball(const InputState &input)
{
  const bool enterOnce = miniGameEnterOnce(input);
  const uint32_t now = millis();

  // If we're actively playing (not reward, not game over), clear accept-arming state.
  // This prevents stale arming timers from carrying across rounds.
  if (!s_showReward && !g_app.gameOver)
  {
    s_acceptArmed = false;
    s_gameOverMs = 0;
  }

  // ---------------------------------------------------------------------------
  // Reward modal: require an "armed" ENTER (prevents instant skip)
  // ---------------------------------------------------------------------------
  if (s_showReward)
  {
    // If we just entered the reward modal, arm acceptance after a short delay.
    if (s_gameOverMs == 0)
    {
      s_acceptArmed = false;
      s_gameOverMs = now + 180;
      mgBeginInputLockout(180);
      clearInputLatch();
      inputForceClear();
      return;
    }

    if (!s_acceptArmed && (int32_t)(now - s_gameOverMs) >= 0)
      s_acceptArmed = true;

    if (enterOnce && s_acceptArmed && !mgInputLockedOut())
    {
      // Reset arming state for the next transition
      s_acceptArmed = false;
      s_gameOverMs = 0;

      exitMiniGameToReturnUi(true);
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // Game over: transition directly into the reward modal (legacy behavior)
  // ---------------------------------------------------------------------------
  if (g_app.gameOver)
  {
    // Apply result + show reward immediately; the reward modal itself is armed
    // (so holding ENTER won't instantly dismiss it).
    mgApplyResultAndShowReward(playerWon);

    // Arm acceptance for the reward modal and swallow any lingering input.
    s_acceptArmed = false;
    s_gameOverMs = 0;
    mgBeginInputLockout(180);
    clearInputLatch();
    inputForceClear();
    return;
  }

  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  if (!s_flappyInited)
  {
    s_flappyInited = true;
    flappyResetWorld(gW, gH);
    s_lastStepMs = millis();
  }

  if (mgPauseIsPaused())
  {
    s_lastStepMs = now;
    return;
  }

  if (mgPauseJustResumedConsume())
  {
    s_lastStepMs = now;
  }

  const uint32_t aliveMs = flappyAliveMsNow(now);
  if (aliveMs >= s_flappyWinMs)
  {
    playerWon = true;
    g_app.gameOver = true;
    requestUIRedraw();
    s_resultShown = true;
    s_flappyPlaying = false;
    soundConfirm();
    return;
  }

  const bool flap = input.mgSelectOnce || input.mgSelectHeld || input.mgUpOnce || input.mgUpHeld;

  const uint32_t stepMs = 16;

  if ((int32_t)(now - s_lastStepMs) >= 0)
  {
    bool flapUsed = false;

    while ((int32_t)(now - s_lastStepMs) >= 0)
    {
      const bool flapThisStep = (flap && !flapUsed);
      flappyStep(gW, gH, flapThisStep);
      if (flapThisStep)
        flapUsed = true;

      s_lastStepMs += stepMs;
      if (g_app.gameOver)
        break;
    }
  }
}

static void drawRewardModal(int gW, int gH)
{
  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(CC_DATUM);

  // ---------------------------------------------------------
  // BIG WIN / LOSE HEADER
  // ---------------------------------------------------------
  spr.setTextColor(playerWon ? TFT_GREEN : TFT_RED, TFT_BLACK);
  spr.drawCentreString(
      playerWon ? "YOU WIN!" : "YOU LOSE!",
      gW / 2,
      gH / 2 - 36,
      4   // big font
  );

  // ---------------------------------------------------------
  // Reward body text (supports 1 or 2 lines)
  // ---------------------------------------------------------
  const char* nl = strchr(s_rewardMsg, '\n');

  if (nl)
  {
    char line1[64];
    size_t len = (size_t)(nl - s_rewardMsg);
    if (len > sizeof(line1) - 1)
      len = sizeof(line1) - 1;

    memcpy(line1, s_rewardMsg, len);
    line1[len] = 0;

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString(line1, gW / 2, gH / 2 - 4, 2);

    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
    spr.drawCentreString(nl + 1, gW / 2, gH / 2 + 16, 2);
  }
  else
  {
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString(s_rewardMsg, gW / 2, gH / 2, 2);
  }

  // ---------------------------------------------------------
  // Footer
  // ---------------------------------------------------------
  spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  spr.drawCentreString("Press ENTER", gW / 2, gH / 2 + 40, 2);
}

void drawFlappyFireball()
{
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  // Background: dark gradient sky (infernal night)
  spr.fillSprite(0x0821); // very dark navy/black
  for (int y = gH - 1; y >= gH / 2; --y) {
    int t = (gH - 1 - y) * 255 / (gH / 2);
    if (t > 255) t = 255;
    uint8_t r5 = (uint8_t)(t * 8 / 255) & 0x1F;
    spr.drawFastHLine(0, y, gW, (uint16_t)(r5 << 11));
  }
  // Stars (static pattern)
  static const uint8_t kStarX[] = { 10, 40, 80, 120, 160, 200, 230, 25, 70, 150, 195 };
  static const uint8_t kStarY[] = {  8, 20,  5,  15,   9,   4,  18, 30, 12,  25,  30 };
  for (int s = 0; s < 11; ++s) {
    spr.drawPixel(kStarX[s], kStarY[s], TFT_WHITE);
  }

  if (s_showReward)
  {
    drawRewardModal(gW, gH);
    return;
  }

  if (g_app.gameOver)
  {
    spr.fillRect(20, gH/2 - 28, gW - 40, 64, TFT_BLACK);
    const uint16_t borderCol = playerWon ? TFT_GREEN : TFT_RED;
    spr.drawRect(20, gH/2 - 28, gW - 40, 64, borderCol);
    spr.drawRect(21, gH/2 - 27, gW - 42, 62, borderCol);
    spr.setTextDatum(CC_DATUM);
    spr.setTextColor(borderCol, TFT_BLACK);
    spr.drawCentreString(playerWon ? "YOU WIN!" : "YOU LOSE!", gW/2, gH/2 - 12, 4);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString(playerWon ? "Survived the flames!" : "Burned to ash...", gW/2, gH/2 + 8, 2);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW/2, gH/2 + 22, 2);
    return;
  }

  // Pipes – dark green stone columns with highlight edge and cap
  const int pipeW = 26;
  const int gapH  = 64;
  const uint16_t pipeBody  = 0x02A0;
  const uint16_t pipeEdgeL = 0x05C0;
  const uint16_t pipeEdgeR = 0x0140;
  const uint16_t pipeCap   = 0x03E0;

  for (int i = 0; i < 3; ++i)
  {
    const int x      = s_pipes[i].x;
    const int gapTop = s_pipes[i].gapY - gapH / 2;
    const int gapBot = s_pipes[i].gapY + gapH / 2;

    if (gapTop > 0) {
      spr.fillRect(x + 1, 0, pipeW - 2, gapTop, pipeBody);
      spr.drawFastVLine(x, 0, gapTop, pipeEdgeL);
      spr.drawFastVLine(x + pipeW - 1, 0, gapTop, pipeEdgeR);
      spr.fillRect(x - 2, gapTop - 6, pipeW + 4, 6, pipeCap);
      spr.drawRect(x - 2, gapTop - 6, pipeW + 4, 6, pipeEdgeR);
    }
    if (gapBot < gH) {
      spr.fillRect(x + 1, gapBot, pipeW - 2, gH - gapBot, pipeBody);
      spr.drawFastVLine(x, gapBot, gH - gapBot, pipeEdgeL);
      spr.drawFastVLine(x + pipeW - 1, gapBot, gH - gapBot, pipeEdgeR);
      spr.fillRect(x - 2, gapBot, pipeW + 4, 6, pipeCap);
      spr.drawRect(x - 2, gapBot, pipeW + 4, 6, pipeEdgeR);
    }
  }

  // Fireball – layered glow effect
  spr.fillCircle(s_fbX, s_fbY, 7, TFT_RED);
  spr.fillCircle(s_fbX, s_fbY, 5, TFT_ORANGE);
  spr.fillCircle(s_fbX, s_fbY, 3, TFT_YELLOW);
  spr.fillCircle(s_fbX, s_fbY, 1, TFT_WHITE);

  // Timer bar – colour transitions green → yellow → red
  {
    const uint32_t now    = millis();
    const uint32_t aliveMs = flappyAliveMsNow(now);
    const uint32_t winMs   = s_flappyWinMs;

    const int barW = gW - 20;
    const int barX = 10;
    const int barY = 4;
    const int barH = 5;

    spr.fillRect(barX, barY, barW, barH, 0x2104);

    int fill = (int)((aliveMs * (uint32_t)(barW - 2)) / winMs);
    if (fill < 0) fill = 0;
    if (fill > barW - 2) fill = barW - 2;

    const int pct = (int)((aliveMs * 100) / winMs);
    const uint16_t barCol = (pct < 50) ? TFT_GREEN : ((pct < 80) ? TFT_YELLOW : TFT_ORANGE);
    spr.fillRect(barX + 1, barY + 1, fill, barH - 2, barCol);
    spr.drawRect(barX, barY, barW, barH, TFT_DARKGREY);
  }
}

// -----------------------------------------------------------------------------
// Random INF reward (WIN only)
// -----------------------------------------------------------------------------
static int rollMiniGameInfReward()
{
  const int r = (int)random(100);

  if (r < 50)
    return 25;
  if (r < 80)
    return 50;
  if (r < 95)
    return 75;
  return 100;
}

// -----------------------------------------------------------------------------
// Mini-game reward (WIN only): 25% chance to win ONE random item
// -----------------------------------------------------------------------------
static bool tryAwardWinItem_1in4(ItemType *outType)
{
  if (outType)
    *outType = ITEM_NONE;

  if (random(4) != 0)
    return false;

  static const ItemType kRewards[] = {ITEM_SOUL_FOOD, ITEM_CURSED_RELIC, ITEM_DEMON_BONE, ITEM_RITUAL_CHALK};

  const int n = (int)(sizeof(kRewards) / sizeof(kRewards[0]));
  const ItemType t = kRewards[(int)random((long)n)];

  g_app.inventory.addItem(t, 1);
  if (outType)
    *outType = t;
  return true;
}

static void mgApplyResultAndShowReward(bool won)
{
  // Old rules (from your reference):
  // WIN  => XP +25, INF +roll, MOOD +20, 25% chance random item +1 (also show name)
  // LOSE => XP +5,  MOOD +10

  if (won)
  {
    pet.addXP(25);

    const int infReward = rollMiniGameInfReward();
    addInf(infReward);

    pet.happiness = constrain(pet.happiness + 20, 0, 100);

    ItemType rewardType = ITEM_NONE;
    const bool wonItem = tryAwardWinItem_1in4(&rewardType);

    if (wonItem)
    {
      const char* nm = mgItemName(rewardType);
      snprintf(
        s_rewardMsg,
        sizeof(s_rewardMsg),
        "You win! XP +25  INF +%d  MOOD +20\nRandom Reward: %s +1",
        infReward,
        (nm && nm[0]) ? nm : "ITEM"
      );
    }
    else
    {
      snprintf(
        s_rewardMsg,
        sizeof(s_rewardMsg),
        "You win! XP +25  INF +%d  MOOD +20",
        infReward
      );
    }
  }
  else
  {
    pet.addXP(5);
    pet.happiness = constrain(pet.happiness + 10, 0, 100);

    snprintf(
      s_rewardMsg,
      sizeof(s_rewardMsg),
      "You lose! XP +5  MOOD +10"
    );
  }

  saveManagerMarkDirty();

  // Show modal (and clear gameOver so we don’t re-enter result logic)
  s_showReward = true;
  g_app.gameOver = false;

  requestUIRedraw();
}

void miniGameExitToReturnUi(bool beginLockout)
{
  s_mgExiting = true;

  s_showReward = false;
  s_rewardMsg[0] = 0;

  // Decide where we're going FIRST, and switch away from MINI_GAME / MG_PAUSE.
  UIState target = miniGameGetReturnUiOrDefault(UIState::PET_SCREEN);
  if (target == UIState::MINI_GAME || target == UIState::MG_PAUSE)
    target = UIState::PET_SCREEN;

  miniGameClearReturnUi();

  // IMPORTANT: switch UI state BEFORE clearing currentMiniGame,
  // so MG_PAUSE doesn't render one frame with "NO MINI GAME".
  g_app.uiState = target;

  // Now tear down the mini-game
  g_app.inMiniGame = false;
  g_app.gameOver   = false;
  playerWon        = false;
  currentMiniGame  = MiniGame::NONE;

  mgPauseReset();
  clearInputLatch();

  // Prevent Exit-confirm ENTER from triggering other UI confirms
  inputForceClear();
  s_prevSelectHeld = false;

  invalidateBackgroundCache();
  requestFullUIRedraw();

  if (beginLockout)
    mgBeginInputLockout(220);
}

// Back-compat: older call sites used this name.
static void exitMiniGameToReturnUi(bool beginLockout) { miniGameExitToReturnUi(beginLockout); }

static void mgSyncGameTimebases(uint32_t now);

// -----------------------------------------------------------------------------
// Universal mini-game update/draw dispatch + pause overlay
// -----------------------------------------------------------------------------

static void mgSyncGameTimebases(uint32_t now);

void updateMiniGame(const InputState &input)
{
  // If we are not on the MINI_GAME UI anymore, never run mini-game logic.
  if (g_app.uiState != UIState::MINI_GAME)
    return;

  if (!g_app.inMiniGame)
    return;

  if (currentMiniGame == MiniGame::NONE)
    return;

  const uint32_t now = millis();

  // Pause/menu logic first (ESC/menu must always work).
  const uint8_t p = mgPauseHandle(input);
  mgPauseUpdateClocks(now);

  if (p == MGPAUSE_EXIT)
  {
    miniGameExitToReturnUi(true);
    requestUIRedraw();
    return;
  }

  if (p == MGPAUSE_CONSUME)
  {
    if (mgPauseIsPaused())
      mgSyncGameTimebases(now);

    requestUIRedraw();
    return;
  }

  if (mgPauseIsPaused())
  {
    mgSyncGameTimebases(now);
    requestUIRedraw();
    return;
  }

  // Gameplay update dispatch
  switch (currentMiniGame)
  {
  case MiniGame::FLAPPY_FIREBALL:
    updateFlappyFireball(input);
    break;

  case MiniGame::CROSSY_ROAD:
    updateCrossyRoad(input);
    break;

  case MiniGame::INFERNAL_DODGER:
    updateInfernalDodger(input);
    break;

  case MiniGame::RESURRECTION:
    updateResurrectionRun(input);
    break;

  default:
    break;
  }

  requestUIRedraw();
}

void drawMiniGame()
{
  if (g_app.uiState != UIState::MINI_GAME)
    return;
  if (!g_app.inMiniGame)
    return;
  if (currentMiniGame == MiniGame::NONE)
    return;

  const uint32_t now = millis();
  mgPauseUpdateClocks(now);

  switch (currentMiniGame)
  {
  case MiniGame::FLAPPY_FIREBALL:
    drawFlappyFireball();
    break;
  case MiniGame::CROSSY_ROAD:
    drawCrossyRoad();
    break;
  case MiniGame::INFERNAL_DODGER:
    drawInfernalDodger();
    break;
  case MiniGame::RESURRECTION:
    drawResurrectionRun();
    break;
  default:
    break;
  }

  if (mgPauseIsPaused())
  {
    mgDrawPauseOverlay();
  }
}

// -----------------------------------------------------------------------------
// Resurrection Run (side-scroller runner)
// -----------------------------------------------------------------------------

static bool rr_active = false;
static bool rr_gameOver = false;
static bool rr_won = false;
static bool rr_ducking = false;

static float rr_y = 0.0f;
static float rr_vy = 0.0f;
static bool rr_onGround = true;

static int rr_distance = 0;
static uint32_t rr_lastMs = 0;

struct RRObs
{
  int x;
  int y;
  int w;
  int h;
  bool active;
};

static RRObs rr_obs[8];
static int rr_courseLen = 2600;

struct RRSpawn
{
  int triggerDist;
  uint8_t type;
  uint8_t param;
};

static const uint8_t RR_SPIKE = 0;
static const uint8_t RR_LOW_FIRE = 1;

static const RRSpawn rr_script[] = {
    {260, RR_SPIKE, 0},  {520, RR_SPIKE, 0},  {780, RR_LOW_FIRE, 0},  {1020, RR_SPIKE, 0}, {1280, RR_LOW_FIRE, 0},
    {1520, RR_SPIKE, 0}, {1780, RR_SPIKE, 0}, {2040, RR_LOW_FIRE, 0}, {2280, RR_SPIKE, 0}, {2540, RR_LOW_FIRE, 0},
};

static const int rr_scriptCount = (int)(sizeof(rr_script) / sizeof(rr_script[0]));
static int rr_nextSpawn = 0;

static void rrSpawnObstacle(uint8_t type)
{
  const int w = (screenW > 0) ? screenW : 240;
  const int h = (screenH > 0) ? screenH : 135;
  int slot = -1;
  for (int i = 0; i < (int)(sizeof(rr_obs) / sizeof(rr_obs[0])); i++)
  {
    if (!rr_obs[i].active)
    {
      slot = i;
      break;
    }
  }
  if (slot < 0)
    return;

  const int spawnScreenLead = 60;
  const int spawnWorldX = rr_distance + w + spawnScreenLead;

  const int groundY = h - 18;

  if (type == RR_SPIKE)
  {
    rr_obs[slot].x = spawnWorldX;
    rr_obs[slot].y = groundY - 14;
    rr_obs[slot].w = 16;
    rr_obs[slot].h = 14;
    rr_obs[slot].active = true;
  }
  else
  {
    rr_obs[slot].x = spawnWorldX;
    rr_obs[slot].y = groundY - 32;
    rr_obs[slot].w = 18;
    rr_obs[slot].h = 10;
    rr_obs[slot].active = true;
  }
}

static void rrResetObstacles()
{
  for (auto &o : rr_obs)
    o = {0, 0, 0, 0, false};
}

static bool rrAabb(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
  return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
}

void startResurrectionRun()
{
  mgPauseReset();
  inputSetTextCapture(false);

  // Make Resurrection Run behave like all other mini-games (state + flags)
  g_app.inMiniGame = true;
  g_app.gameOver = false;
  playerWon = false;
  s_resultShown = false;

  currentMiniGame = MiniGame::RESURRECTION;

  // Never allow "return UI" to be MINI_GAME / MG_PAUSE (causes exit->bounce/lock).
  UIState retUi = g_app.uiState;
  if (retUi == UIState::MINI_GAME || retUi == UIState::MG_PAUSE)
    retUi = UIState::PET_SCREEN;

  miniGameSetReturnUi(retUi);
  uiActionEnterState(UIState::MINI_GAME, g_app.currentTab, false);

  // Hard reset any previous end-of-game modal state so retries don't instantly re-trigger.
  s_showReward = false;
  s_rewardMsg[0] = 0;

  s_prevSelectHeld = false;
  clearInputLatch();
  mgBeginInputLockout(220);

  rr_active = true;
  rr_gameOver = false;
  rr_won = false;
  rr_ducking = false;

  rr_distance = 0;
  rr_lastMs = millis();

  rr_y = 0.0f;
  rr_vy = 0.0f;
  rr_onGround = true;

  rr_courseLen = 2600;
  rrResetObstacles();
  rr_nextSpawn = 0;

  invalidateBackgroundCache();
  requestUIRedraw();
}

void updateResurrectionRun(const InputState &input)
{
  const uint32_t now = millis();

  // Clear accept-arming state while actively playing
  if (!rr_gameOver)
  {
    s_acceptArmed = false;
    s_gameOverMs = 0;
  }

  if (rr_gameOver)
  {
    const bool enterOnce = miniGameEnterOnce(input);

    // First frame of game-over: arm acceptance after a short delay and swallow input
    if (s_gameOverMs == 0)
    {
      s_acceptArmed = false;
      s_gameOverMs = now + 180;
      mgBeginInputLockout(180);
      clearInputLatch();
      inputForceClear();
      return;
    }

    if (!s_acceptArmed && (int32_t)(now - s_gameOverMs) >= 0)
      s_acceptArmed = true;

    if (enterOnce && s_acceptArmed && !mgInputLockedOut())
    {
      // Consume accept so it can't double-trigger
      s_acceptArmed = false;
      s_gameOverMs = 0;

      rr_active = false;
      rr_gameOver = false;

      playerWon = rr_won;

      currentMiniGame = MiniGame::NONE;
      g_app.inMiniGame = false;

      onResurrectionMiniGameResult(playerWon);

      miniGameClearReturnUi();

      mgPauseReset();
      clearInputLatch();
      inputForceClear();
      requestUIRedraw();
      mgBeginInputLockout(220);
    }
    return;
  }

  uint32_t dtMs = now - rr_lastMs;
  rr_lastMs = now;
  if (dtMs > 40)
    dtMs = 40;
  const float dt = dtMs / 1000.0f;

  rr_ducking = (input.mgDownHeld || input.mgSpaceHeld);

  const bool jumpOnce = input.mgSelectOnce || input.mgUpOnce;
  const bool jumpHeld = input.mgSelectHeld || input.mgUpHeld;

  if (jumpOnce && rr_onGround)
  {
    rr_vy = -220.0f;
    rr_onGround = false;
  }

  const float gravity = (jumpHeld && rr_vy < 0.0f) ? 520.0f : 720.0f;

  rr_vy += gravity * dt;
  rr_y += rr_vy * dt;

  if (rr_y >= 0.0f)
  {
    rr_y = 0.0f;
    rr_vy = 0.0f;
    rr_onGround = true;
  }

  const int speed = 290;
  rr_distance += (int)(speed * dt);

  while (rr_nextSpawn < rr_scriptCount && rr_distance >= rr_script[rr_nextSpawn].triggerDist)
  {
    rrSpawnObstacle(rr_script[rr_nextSpawn].type);
    rr_nextSpawn++;
  }

  if (rr_distance >= rr_courseLen)
  {
    rr_gameOver = true;
    rr_won = true;
    return;
  }

  const int w = (screenW > 0) ? screenW : 240;
  const int h = (screenH > 0) ? screenH : 135;

  const int groundY = h - 18;
  const int px = 48;
  int py = groundY - 18 + (int)rr_y;
  int pw = 16;
  int ph = rr_ducking ? 10 : 16;
  if (rr_ducking)
    py = groundY - ph + (int)rr_y;

  for (auto &o : rr_obs)
  {
    if (!o.active)
      continue;

    int ox = o.x - rr_distance;
    int oy = o.y;
    if (rrAabb(px, py, pw, ph, ox, oy, o.w, o.h))
    {
      rr_gameOver = true;
      rr_won = false;
      return;
    }

    if (ox < -40)
      o.active = false;
  }
}

void drawResurrectionRun()
{
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  // Background: dark sky with purple hue (underworld)
  spr.fillSprite(0x1008); // dark purple-grey

  // Stars / floating embers in background
  static const uint8_t kRrStarX[] = { 15, 50, 90, 130, 170, 210, 35, 100, 175, 220 };
  static const uint8_t kRrStarY[] = { 12,  8, 20,   5,  14,  10, 28,  30,  22,  25 };
  for (int s = 0; s < 10; ++s) {
    spr.drawPixel(kRrStarX[s], kRrStarY[s], 0x8410); // dim grey-white
  }

  const int groundY = gH - 18;

  // Ground texture: dark earth with grass-line
  spr.fillRect(0, groundY, gW, gH - groundY, 0x2100); // dark brown earth
  spr.drawFastHLine(0, groundY, gW, 0x0440);           // dark green grass line
  // Grass tufts (static pattern that scrolls would need animation; static is fine)
  for (int gx = (rr_distance % 20); gx < gW; gx += 20) {
    spr.drawPixel(gx,     groundY - 1, 0x0660);
    spr.drawPixel(gx + 2, groundY - 2, 0x0660);
    spr.drawPixel(gx + 4, groundY - 1, 0x0660);
  }

  // Game-over screen
  if (rr_gameOver) {
    spr.fillRect(20, gH/2 - 28, gW - 40, 64, TFT_BLACK);
    const uint16_t bc = rr_won ? TFT_GREEN : TFT_RED;
    spr.drawRect(20, gH/2 - 28, gW - 40, 64, bc);
    spr.drawRect(21, gH/2 - 27, gW - 42, 62, bc);

    spr.setTextDatum(CC_DATUM);
    spr.setTextColor(bc, TFT_BLACK);
    spr.drawCentreString(rr_won ? "RESURRECTED!" : "FALLEN...", gW/2, gH/2 - 12, 4);

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString(rr_won ? "The soul lives on!" : "Lost to the void.", gW/2, gH/2 + 8, 2);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW/2, gH/2 + 22, 2);
    return;
  }

  // Player: ghost/skeleton silhouette
  int px = 48;
  int py = groundY - 18 + (int)rr_y;
  int pw = 14;
  int ph = rr_ducking ? 9 : 16;
  if (rr_ducking) py = groundY - ph + (int)rr_y;

  if (!rr_ducking) {
    // Body
    spr.fillRect(px + 2, py + 6, pw - 4, ph - 6, 0xBDF7);   // light ghostly white
    // Head (rounded top)
    spr.fillCircle(px + pw/2, py + 4, 5, 0xDEFB);
    // Eyes (red-ish)
    spr.fillRect(px + 3, py + 2, 2, 2, TFT_RED);
    spr.fillRect(px + 8, py + 2, 2, 2, TFT_RED);
  } else {
    // Ducking: flatter rectangle
    spr.fillRect(px + 1, py, pw - 2, ph, 0xBDF7);
    spr.fillRect(px + 3, py + 2, 2, 2, TFT_RED);
    spr.fillRect(px + 7, py + 2, 2, 2, TFT_RED);
  }

  for (auto &o : rr_obs)
  {
    if (!o.active)
      continue;
    int ox = o.x - rr_distance;
    if (ox < -40 || ox > gW + 40) continue;

    // Detect type by height: SPIKE=14h, LOW_FIRE=10h
    if (o.h >= 12) {
      // SPIKE: bone/tooth spike shape
      spr.fillRect(ox + 4, o.y + 4, o.w - 8, o.h - 4, 0xAD55);  // bone beige
      spr.fillTriangle(ox, o.y + o.h, ox + o.w/2, o.y, ox + o.w, o.y + o.h, 0xDEFB); // spike tip
      spr.drawTriangle(ox, o.y + o.h, ox + o.w/2, o.y, ox + o.w, o.y + o.h, TFT_DARKGREY);
    } else {
      // LOW_FIRE: small flame
      spr.fillCircle(ox + o.w/2, o.y + o.h/2, o.w/2, TFT_ORANGE);
      spr.fillCircle(ox + o.w/2, o.y + o.h/2 - 2, o.w/2 - 2, TFT_YELLOW);
      spr.fillCircle(ox + o.w/2, o.y + o.h/2 - 3, 2, TFT_WHITE);
    }
  }

  // Progress bar
  const int barW = gW - 20;
  const int barX = 10;
  const int barY = 4;
  spr.fillRect(barX, barY, barW, 5, 0x2104);
  int fill = (rr_distance * (barW - 2)) / rr_courseLen;
  if (fill < 0) fill = 0;
  if (fill > barW - 2) fill = barW - 2;

  // Colour: purple → blue → green as you approach win
  const int pct = (rr_distance * 100) / rr_courseLen;
  uint16_t barCol = (pct < 50) ? 0xF81F : ((pct < 80) ? TFT_CYAN : TFT_GREEN); // magenta→cyan→green
  spr.fillRect(barX + 1, barY + 1, fill, 3, barCol);
  spr.drawRect(barX, barY, barW, 5, TFT_DARKGREY);
}

// -----------------------------------------------------------------------------
// CROSSY ROAD (Frogger-lite)
// -----------------------------------------------------------------------------

static const int kCrossyCols = 15;
static const int kCrossyRows = 9;

static const int kCrossyTileW = 16;
static const int kCrossyTileH = 15;

static const int kCrossyOriginX = 0;
static const int kCrossyOriginY = 0;

static const int kCrossyFirstTrafficRow = 1;
static const int kCrossyLastTrafficRow = kCrossyRows - 2;

struct CrossyLane
{
  int8_t dir;
  uint8_t speed;
  uint8_t carLen;
  uint16_t gapPx;
  int32_t offsetPx;
};

static CrossyLane s_crossyLanes[kCrossyRows];

static int s_crossyPx = 0;
static int s_crossyPy = 0;

static bool s_crossyInited = false;
static uint32_t s_crossyLastLaneMs = 0;

static inline int crossyClamp(int v, int lo, int hi)
{
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

static void crossyInitLanes()
{
  memset(s_crossyLanes, 0, sizeof(s_crossyLanes));

  for (int r = kCrossyFirstTrafficRow; r <= kCrossyLastTrafficRow; ++r)
  {
    CrossyLane &L = s_crossyLanes[r];
    L.dir = (r % 2 == 0) ? +1 : -1;
    L.speed = (uint8_t)(1 + (r % 2));
    L.carLen = (uint8_t)(1 + (r % 2));
    L.gapPx = (uint16_t)(kCrossyTileW * (4 + (r % 3)));
    L.offsetPx = (int32_t)random((long)(kCrossyTileW * 6));
  }
}

static void crossyReset()
{
  s_crossyPx = kCrossyCols / 2;
  s_crossyPy = kCrossyRows - 1;

  s_crossyLastLaneMs = millis();

  crossyInitLanes();
}

void startCrossyRoad()
{
  mgPauseReset();
  inputSetTextCapture(false);

  g_app.inMiniGame = true;
  g_app.gameOver = false;
  playerWon = false;
  s_resultShown = false;

  s_showReward = false;
  s_rewardMsg[0] = 0;

  currentMiniGame = MiniGame::CROSSY_ROAD;

  // Never allow "return UI" to be MINI_GAME / MG_PAUSE (causes exit->bounce/lock).
  UIState retUi = g_app.uiState;
  if (retUi == UIState::MINI_GAME || retUi == UIState::MG_PAUSE)
    retUi = UIState::PET_SCREEN;

  miniGameSetReturnUi(retUi);
  uiActionEnterState(UIState::MINI_GAME, g_app.currentTab, false);

  s_crossyInited = true;
  crossyReset();

  invalidateBackgroundCache();
  requestUIRedraw();
  clearInputLatch();
  mgBeginInputLockout(220);
}

static void crossyStepLanes(uint32_t now)
{
  const uint32_t kLaneStepMs = 50;
  if ((uint32_t)(now - s_crossyLastLaneMs) < kLaneStepMs)
    return;
  s_crossyLastLaneMs = now;

  for (int r = kCrossyFirstTrafficRow; r <= kCrossyLastTrafficRow; ++r)
  {
    CrossyLane &L = s_crossyLanes[r];
    L.offsetPx += (int32_t)L.speed * (L.dir > 0 ? 1 : -1);
  }
}

static bool crossyHitCar()
{
  if (s_crossyPy < kCrossyFirstTrafficRow || s_crossyPy > kCrossyLastTrafficRow)
    return false;

  const CrossyLane &L = s_crossyLanes[s_crossyPy];

  const int carLenPx = (int)L.carLen * kCrossyTileW;
  const int periodPx = carLenPx + (int)L.gapPx;

  const int laneW = kCrossyCols * kCrossyTileW;

  const int playerX0 = s_crossyPx * kCrossyTileW;
  const int playerX1 = playerX0 + (kCrossyTileW - 1);

  for (int testX = playerX0; testX <= playerX1; testX += (kCrossyTileW / 2))
  {
    int shifted = testX + (int)L.offsetPx;
    int m = shifted % periodPx;
    if (m < 0)
      m += periodPx;

    if (testX < 0 || testX >= laneW)
      continue;

    if (m >= 0 && m < carLenPx)
      return true;
  }

  return false;
}

void updateCrossyRoad(const InputState &input)
{
  const bool enterOnce = miniGameEnterOnce(input);
  const uint32_t now = millis();

  // Clear accept-arming state while actively playing
  if (!s_showReward && !g_app.gameOver)
  {
    s_acceptArmed = false;
    s_gameOverMs = 0;
  }

  // ---------------------------------------------------------------------------
  // Reward modal: require an "armed" ENTER (prevents instant skip)
  // ---------------------------------------------------------------------------
  if (s_showReward)
  {
    // First frame of reward modal: arm acceptance after a short delay and swallow input
    if (s_gameOverMs == 0)
    {
      s_acceptArmed = false;
      s_gameOverMs = now + 180;
      mgBeginInputLockout(180);
      clearInputLatch();
      inputForceClear();
      return;
    }

    if (!s_acceptArmed && (int32_t)(now - s_gameOverMs) >= 0)
      s_acceptArmed = true;

    if (enterOnce && s_acceptArmed && !mgInputLockedOut())
    {
      s_acceptArmed = false;
      s_gameOverMs = 0;

      exitMiniGameToReturnUi(true);
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // Game over: transition directly into the reward modal (legacy behavior)
  // ---------------------------------------------------------------------------
  if (g_app.gameOver)
  {
    // Apply result + show reward immediately; the reward modal itself is armed
    // (so holding ENTER won't instantly dismiss it).
    mgApplyResultAndShowReward(playerWon);

    // Arm acceptance for the reward modal and swallow any lingering input.
    s_acceptArmed = false;
    s_gameOverMs = 0;
    mgBeginInputLockout(180);
    clearInputLatch();
    inputForceClear();
    return;
  }
  if (!s_crossyInited)
  {
    s_crossyInited = true;
    crossyReset();
  }
  crossyStepLanes(now);

  int dx = 0, dy = 0;

  if (input.mgLeftOnce)
    dx = -1;
  if (input.mgRightOnce)
    dx = +1;
  if (input.mgUpOnce)
    dy = -1;
  if (input.mgDownOnce)
    dy = +1;

  if (input.encoderDelta < 0)
    dy = -1;
  if (input.encoderDelta > 0)
    dy = +1;

  if (dx || dy)
  {
    s_crossyPx = crossyClamp(s_crossyPx + dx, 0, kCrossyCols - 1);
    s_crossyPy = crossyClamp(s_crossyPy + dy, 0, kCrossyRows - 1);
    playBeep();
  }

  if (s_crossyPy == 0)
  {
    playerWon = true;
    g_app.gameOver = true;
    requestUIRedraw();
    s_resultShown = true;
    soundConfirm();
    return;
  }

  if (crossyHitCar())
  {
    playerWon = false;
    g_app.gameOver = true;
    requestUIRedraw();
    s_resultShown = true;
    soundError();
    return;
  }
}

void drawCrossyRoad()
{
  if (!s_crossyInited)
  {
    s_crossyInited = true;
    crossyReset();
  }

  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;
  (void)gH;

  spr.fillSprite(TFT_BLACK);

  if (s_showReward)
  {
    drawRewardModal(gW, gH);
    return;
  }

  if (g_app.gameOver)
  {
    spr.setTextDatum(CC_DATUM);
    spr.setTextColor(playerWon ? TFT_GREEN : TFT_RED, TFT_BLACK);
    spr.drawCentreString(playerWon ? "YOU WIN!" : "YOU LOSE!", gW / 2, gH / 2 - 10, 4);

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW / 2, gH / 2 + 22, 2);
    return;
  }

  // Draw rows with themed colours
  for (int y = 0; y < kCrossyRows; ++y) {
    const int ry = kCrossyOriginY + y * kCrossyTileH;
    const int rw = kCrossyCols * kCrossyTileW;

    if (y == 0) {
      // Goal: vivid green grass with small markers
      spr.fillRect(kCrossyOriginX, ry, rw, kCrossyTileH, 0x04A0);
      spr.drawFastHLine(kCrossyOriginX, ry + kCrossyTileH - 1, rw, 0x0780);
      for (int mx = 8; mx < rw - 8; mx += 24) {
        spr.fillRect(kCrossyOriginX + mx, ry + 4, 8, kCrossyTileH - 8, 0x07E0);
      }
    } else if (y == kCrossyRows - 1) {
      // Start: concrete sidewalk with tile lines
      spr.fillRect(kCrossyOriginX, ry, rw, kCrossyTileH, 0x7BEF);
      for (int sx = 0; sx < rw; sx += 16) {
        spr.drawFastVLine(kCrossyOriginX + sx, ry, kCrossyTileH, 0x5AEB);
      }
    } else {
      // Traffic lane: dark asphalt with dashed centre line
      spr.fillRect(kCrossyOriginX, ry, rw, kCrossyTileH, 0x2124);
      const int cy = ry + kCrossyTileH / 2;
      for (int sx = 4; sx < rw - 4; sx += 14) {
        spr.drawFastHLine(kCrossyOriginX + sx, cy, 8, 0xFFE0);
      }
    }
  }

  // Cars – body + windows + wheel dots
  for (int r = kCrossyFirstTrafficRow; r <= kCrossyLastTrafficRow; ++r) {
    const CrossyLane& L = s_crossyLanes[r];

    const int carLenPx = (int)L.carLen * kCrossyTileW;
    const int periodPx = carLenPx + (int)L.gapPx;
    const int laneW = kCrossyCols * kCrossyTileW;

    // Safety: never allow modulo/step by 0 or negative
    if (carLenPx <= 0 || periodPx <= 0) continue;

    const uint16_t carCol = (r % 3 == 0) ? TFT_RED :
                            (r % 3 == 1) ? 0x001F : 0x8000;
    const uint16_t winCol = 0xAEFC;

    const int y = kCrossyOriginY + r * kCrossyTileH;

    int offset = (int)(L.offsetPx % periodPx);
    if (offset < 0)
      offset += periodPx;

    for (int x0 = -periodPx * 2; x0 < laneW + periodPx * 2; x0 += periodPx) {
      const int x = x0 - offset;
      const int drawX = kCrossyOriginX + x;

      if (drawX + carLenPx < kCrossyOriginX)
        continue;
      if (drawX > kCrossyOriginX + laneW)
        continue;

      // Car body
      spr.fillRect(drawX, y + 2, carLenPx - 1, kCrossyTileH - 4, carCol);
      // Window
      if (carLenPx > 12) {
        spr.fillRect(drawX + 2, y + 3, carLenPx / 3, kCrossyTileH - 8, winCol);
      }
      // Top highlight
      spr.drawFastHLine(drawX + 1, y + 2, carLenPx - 3, TFT_WHITE);
      // Wheel dots
      spr.fillRect(drawX + 1,            y + kCrossyTileH - 5, 3, 3, TFT_BLACK);
      spr.fillRect(drawX + carLenPx - 4, y + kCrossyTileH - 5, 3, 3, TFT_BLACK);
    }
  }

  // Player: little imp with horns and eyes
  const int fx = kCrossyOriginX + s_crossyPx * kCrossyTileW;
  const int fy = kCrossyOriginY + s_crossyPy * kCrossyTileH;
  const int tw = kCrossyTileW;
  const int th = kCrossyTileH;

  spr.fillRect(fx + 3, fy + 4, tw - 6, th - 7, TFT_GREEN);
  spr.fillCircle(fx + tw/2, fy + 3, 4, 0x07C0);
  // Horns
  spr.fillTriangle(fx + 3, fy + 1, fx + 5, fy - 2, fx + 7, fy + 1, TFT_RED);
  spr.fillTriangle(fx + tw - 7, fy + 1, fx + tw - 5, fy - 2, fx + tw - 3, fy + 1, TFT_RED);
  // Eyes
  spr.fillRect(fx + tw/2 - 3, fy + 2, 2, 2, TFT_BLACK);
  spr.fillRect(fx + tw/2 + 1, fy + 2, 2, 2, TFT_BLACK);
}

// -----------------------------------------------------------------------------
// INFERNAL DODGER
// -----------------------------------------------------------------------------

struct DodgerBall
{
  int16_t x;
  int16_t y;
  int16_t vy;
  uint8_t r;
  bool active;
};

static bool s_dodgerInited = false;
static uint32_t s_dodgerLastStepMs = 0;
static uint32_t s_dodgerStartMs = 0;
static uint32_t s_dodgerSpawnAccMs = 0;

static int16_t s_dodgerPx = 0;
static int16_t s_dodgerPy = 0;
static int16_t s_dodgerSpeed = 3;
static float s_dodgerPxF = 0.0f;
static uint32_t s_dodgerMoveLastMs = 0;

static int8_t s_dodgerMoveDir = 0;
static uint32_t s_dodgerDirHoldMs = 0;

static DodgerBall s_dodgerBalls[8];

static void dodgerReset()
{
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  s_dodgerPx = gW / 2;
  s_dodgerPxF = (float)s_dodgerPx;
  s_dodgerMoveLastMs = millis();
  s_dodgerPy = gH - 14;
  s_dodgerSpeed = 3;

  for (auto &b : s_dodgerBalls)
  {
    b.x = 0;
    b.y = -200;
    b.vy = 2;
    b.r = 4;
    b.active = false;
  }

  s_dodgerStartMs = millis();
  s_dodgerLastStepMs = millis();
  s_dodgerSpawnAccMs = 0;
}

static void dodgerSpawnOne(int difficulty)
{
  const int gW = (screenW > 0) ? screenW : 240;

  int slot = -1;
  for (int i = 0; i < (int)(sizeof(s_dodgerBalls) / sizeof(s_dodgerBalls[0])); ++i)
  {
    if (!s_dodgerBalls[i].active)
    {
      slot = i;
      break;
    }
  }
  if (slot < 0)
    return;

  DodgerBall &b = s_dodgerBalls[slot];

  const int margin = 6;
  b.r = (uint8_t)(3 + (difficulty % 3));
  b.x = (int16_t)random((long)(margin), (long)(gW - margin));
  b.y = (int16_t)(-(int)(10 + random(40)));

  b.vy = (int16_t)(2 + (difficulty / 5));
  if (b.vy > 7)
    b.vy = 7;

  b.active = true;
}

static inline bool dodgerHit(int ax, int ay, int ar, int bx, int by, int br)
{
  const int dx = ax - bx;
  const int dy = ay - by;
  const int rr = ar + br;
  return (dx * dx + dy * dy) <= (rr * rr);
}

static inline uint32_t dodgerAliveMsNow(uint32_t now)
{
  uint32_t elapsed = now - s_dodgerStartMs;

  const uint32_t pausedAccum = mgPauseAccumMs();
  if (elapsed > pausedAccum)
    elapsed -= pausedAccum;
  else
    elapsed = 0;

  if (mgPauseIsPaused() && mgPauseStartMs() != 0)
  {
    uint32_t pausedSoFar = now - mgPauseStartMs();
    if (elapsed > pausedSoFar)
      elapsed -= pausedSoFar;
    else
      elapsed = 0;
  }

  return elapsed;
}

void startInfernalDodger() {
  inputSetTextCapture(false);
  mgPauseReset();

  g_app.inMiniGame = true;
  g_app.gameOver = false;
  playerWon = false;
  s_resultShown = false;

  s_showReward = false;
  s_rewardMsg[0] = 0;

  s_prevSelectHeld = false;

  currentMiniGame = MiniGame::INFERNAL_DODGER;

  // Never allow "return UI" to be MINI_GAME / MG_PAUSE (causes exit->bounce/lock).
  UIState retUi = g_app.uiState;
  if (retUi == UIState::MINI_GAME || retUi == UIState::MG_PAUSE)
    retUi = UIState::PET_SCREEN;

  miniGameSetReturnUi(retUi);
  uiActionEnterState(UIState::MINI_GAME, g_app.currentTab, false);

  s_dodgerInited = false;

  invalidateBackgroundCache();
  requestUIRedraw();
  clearInputLatch();
  // Prevent the ENTER used to launch the mini-game from being interpreted as
  // an immediate "enterOnce" inside the mini-game on the first update tick.
  {
    auto st = M5Cardputer.Keyboard.keysState();
    s_prevSelectHeld = st.enter;
  }
  mgBeginInputLockout(220);
}

void updateInfernalDodger(const InputState &input)
{
  const bool enterOnce = miniGameEnterOnce(input);

  if (s_showReward)
  {
    if (enterOnce)
    {
      exitMiniGameToReturnUi(true);
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // Game over: transition directly into the reward modal (legacy behavior)
  // ---------------------------------------------------------------------------
  if (g_app.gameOver)
  {
    mgApplyResultAndShowReward(playerWon);

    s_acceptArmed = false;
    s_gameOverMs = 0;
    mgBeginInputLockout(180);
    clearInputLatch();
    inputForceClear();
    return;
  }
  if (!s_dodgerInited)
  {
    s_dodgerInited = true;
    dodgerReset();
  }

  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  const uint32_t now = millis();
  const uint32_t aliveMs = dodgerAliveMsNow(now);
  const int difficulty = (int)(aliveMs / 3000);

  const uint32_t kWinMs = kSurviveWinMs;
  if (aliveMs >= kWinMs)
  {
    playerWon = true;
    g_app.gameOver = true;
    requestUIRedraw();
    s_resultShown = true;
    soundConfirm();
    return;
  }

  const bool leftHeld = input.mgLeftHeld;
  const bool rightHeld = input.mgRightHeld;

  if (input.mgLeftOnce)
  {
    s_dodgerMoveDir = -1;
    s_dodgerDirHoldMs = now + 140;
  }
  if (input.mgRightOnce)
  {
    s_dodgerMoveDir = +1;
    s_dodgerDirHoldMs = now + 140;
  }

  if (leftHeld && !rightHeld)
  {
    s_dodgerMoveDir = -1;
    s_dodgerDirHoldMs = now + 140;
  }
  if (rightHeld && !leftHeld)
  {
    s_dodgerMoveDir = +1;
    s_dodgerDirHoldMs = now + 140;
  }

  if (!leftHeld && !rightHeld)
  {
    if ((int32_t)(now - s_dodgerDirHoldMs) >= 0)
      s_dodgerMoveDir = 0;
  }

  if (input.encoderDelta < 0)
  {
    s_dodgerMoveDir = -1;
    s_dodgerDirHoldMs = now + 140;
  }
  if (input.encoderDelta > 0)
  {
    s_dodgerMoveDir = +1;
    s_dodgerDirHoldMs = now + 140;
  }

  uint32_t mvDtMs = now - s_dodgerMoveLastMs;
  s_dodgerMoveLastMs = now;
  if (mvDtMs > 40)
    mvDtMs = 40;

  float pxPerSec = 120.0f + (float)(difficulty * 6);
  if (pxPerSec > 170.0f)
    pxPerSec = 170.0f;

  const float dt = (float)mvDtMs / 1000.0f;
  s_dodgerPxF += (float)s_dodgerMoveDir * pxPerSec * dt;

  const float marginF = 6.0f;
  if (s_dodgerPxF < marginF)
    s_dodgerPxF = marginF;
  if (s_dodgerPxF > (float)gW - marginF)
    s_dodgerPxF = (float)gW - marginF;

  s_dodgerPx = (int16_t)(s_dodgerPxF + 0.5f);

  const int margin = 6;
  if (s_dodgerPx < margin)
    s_dodgerPx = margin;
  if (s_dodgerPx > gW - margin)
    s_dodgerPx = gW - margin;

  const uint32_t stepMs = 16;
  while ((int32_t)(now - s_dodgerLastStepMs) >= 0)
  {
    int spawnEveryMs = 520 - difficulty * 24;
    if (spawnEveryMs < 220)
      spawnEveryMs = 220;

    s_dodgerSpawnAccMs += stepMs;
    if (s_dodgerSpawnAccMs >= (uint32_t)spawnEveryMs)
    {
      s_dodgerSpawnAccMs = 0;
      dodgerSpawnOne(difficulty);
    }

    for (auto &b : s_dodgerBalls)
    {
      if (!b.active)
        continue;

      b.y += b.vy;

      if (b.y > gH + 12)
      {
        b.active = false;
        continue;
      }

      const int pr = 6;
      if (dodgerHit((int)s_dodgerPx, (int)s_dodgerPy, pr, (int)b.x, (int)b.y, (int)b.r))
      {
        playerWon = false;
        g_app.gameOver = true;
        requestUIRedraw();
        s_resultShown = true;
        soundError();
        return;
      }
    }

    s_dodgerLastStepMs += stepMs;
  }
}

void drawInfernalDodger()
{
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  // Background: infernal dark-red gradient
  spr.fillSprite(0x2000); // dark maroon
  for (int y = gH - 1; y >= gH * 3 / 4; --y) {
    int t = (gH - 1 - y) * 255 / (gH / 4);
    if (t > 255) t = 255;
    uint8_t r5 = (uint8_t)(8 + t * 14 / 255) & 0x1F;
    uint16_t col = (r5 << 11);
    spr.drawFastHLine(0, y, gW, col);
  }

  // Lava/fire border at bottom
  const int lavaY = gH - 10;
  spr.fillRect(0, lavaY, gW, 10, 0xF800); // red lava strip
  for (int fx2 = 0; fx2 < gW; fx2 += 8) {
    spr.fillTriangle(fx2,     lavaY,
                     fx2 + 4, lavaY - 5,
                     fx2 + 8, lavaY,
                     TFT_ORANGE);
  }

  // Ash/ember particles (static ambient)
  static const uint8_t kEmX[] = { 20, 60, 100, 140, 180, 220, 45, 120, 190 };
  static const uint8_t kEmY[] = { 20, 40,  15,  50,  25,  35, 55,  30,  45 };
  for (int e = 0; e < 9; ++e) {
    spr.drawPixel(kEmX[e], kEmY[e], TFT_ORANGE);
  }

  // Progress bar – red filling like a rising lava meter
  {
    const uint32_t aliveMs = millis() - s_dodgerStartMs;
    const uint32_t kWinMs  = 18000;

    const int barW = gW - 20;
    const int barX = 10;
    const int barY = 4;
    const int barH = 5;

    spr.fillRect(barX, barY, barW, barH, 0x2104);

    int fill = (int)((aliveMs * (uint32_t)(barW - 2)) / kWinMs);
    if (fill < 0) fill = 0;
    if (fill > barW - 2) fill = barW - 2;

    // Lava colour: dark orange → bright orange → yellow at end
    const int pct = (int)((aliveMs * 100) / kWinMs);
    const uint16_t barCol = (pct < 60) ? TFT_ORANGE : ((pct < 85) ? 0xFD00 : TFT_YELLOW);
    spr.fillRect(barX + 1, barY + 1, fill, barH - 2, barCol);
    spr.drawRect(barX, barY, barW, barH, TFT_DARKGREY);
  }

  // Reward modal (win only)
  if (s_showReward)
  {
    drawRewardModal(gW, gH);
    return;
  }

  // Win/Lose screen
  if (g_app.gameOver)
  {
    spr.fillRect(20, gH/2 - 28, gW - 40, 64, TFT_BLACK);
    const uint16_t bc = playerWon ? TFT_GREEN : TFT_RED;
    spr.drawRect(20, gH/2 - 28, gW - 40, 64, bc);
    spr.drawRect(21, gH/2 - 27, gW - 42, 62, bc);

    spr.setTextDatum(CC_DATUM);
    spr.setTextColor(bc, TFT_BLACK);
    spr.drawCentreString(playerWon ? "YOU WIN!" : "YOU LOSE!", gW/2, gH/2 - 12, 4);

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString(playerWon ? "Dodged the inferno!" : "Consumed by flames!", gW/2, gH/2 + 8, 2);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW/2, gH/2 + 22, 2);
    return;
  }

  // Draw falling fireballs with layered glow
  for (auto &b : s_dodgerBalls)
  {
    if (!b.active) continue;
    if (b.r > 1) spr.fillCircle(b.x, b.y, b.r + 2, 0x6000); // outer dark halo
    spr.fillCircle(b.x, b.y, b.r, TFT_ORANGE);
    if (b.r > 2) spr.fillCircle(b.x, b.y, b.r - 1, TFT_YELLOW);
    spr.fillCircle(b.x, b.y, 1, TFT_WHITE);
  }

  // Player: small devil sprite (head + horns + body)
  const int px = s_dodgerPx;
  const int py = s_dodgerPy;

  // Body
  spr.fillCircle(px, py, 6, TFT_GREEN);
  spr.drawCircle(px, py, 6, 0x0300); // dark green outline
  // Horns
  spr.fillTriangle(px - 4, py - 5, px - 6, py - 10, px - 2, py - 5, TFT_RED);
  spr.fillTriangle(px + 4, py - 5, px + 6, py - 10, px + 2, py - 5, TFT_RED);
  // Eyes
  spr.fillRect(px - 3, py - 2, 2, 2, TFT_BLACK);
  spr.fillRect(px + 1, py - 2, 2, 2, TFT_BLACK);
  // Tail indicator (small dot below)
  spr.fillCircle(px, py + 8, 2, TFT_RED);
}

static void mgSyncGameTimebases(uint32_t now)
{
  switch (currentMiniGame)
  {
  case MiniGame::FLAPPY_FIREBALL:
    s_lastStepMs = now;
    break;

  case MiniGame::INFERNAL_DODGER:
    s_dodgerLastStepMs = now;
    s_dodgerMoveLastMs = now;
    break;

  case MiniGame::CROSSY_ROAD:
    s_crossyLastLaneMs = now;
    break;

  case MiniGame::RESURRECTION:
    rr_lastMs = now;
    break;

  default:
    break;
  }
}

#endif // RH_MINIGAMES_IMPL_IN_PAUSE_MENU