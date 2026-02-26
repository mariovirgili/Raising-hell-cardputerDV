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

#include "app_state.h"              // g_app
#include "ui_defs.h"                // UIState
#include "ui_runtime.h"             // requestUIRedraw()
#include "graphics.h"               // spr, screenW/screenH, invalidateBackgroundCache() (whatever your project defines here)
#include "sound.h"                  // soundFlap/soundConfirm/soundError/playBeep (or the correct header that declares these)
#include "display.h" 

#include "mini_game_return_ui.h"    // miniGameSetReturnUi / miniGameGetReturnUiOrDefault / miniGameClearReturnUi
#include "mg_pause_core.h"          // mgPause* + MGPAUSE_* constants (see section 2 if you don't have this header yet)
#include "inventory.h"   // defines enum ItemType
#include "pet.h"
#include "currency.h"
#include "save_manager.h"
#include "mg_pause_core.h"
#include "mg_pause_menu.h"
#include "graphics.h"

// -----------------------------------------------------------------------------
// Mini-game input helpers / shared state
// -----------------------------------------------------------------------------

static bool s_acceptArmed = false;
static uint32_t s_gameOverMs = 0;

static bool s_showReward = false;
static char s_rewardMsg[64] = {0};

// Forward decl (used by multiple mini-games)
static int rollMiniGameInfReward();
static void exitMiniGameToReturnUi(bool beginLockout = true);

// Crossy Road
void startCrossyRoad();
void updateCrossyRoad(const InputState& input);
void drawCrossyRoad();

// IMPORTANT:
// We synthesize "Enter once" from selectHeld edge. Do NOT reset this to false
// during transitions, or holding Enter will instantly auto-dismiss the next screen.
static bool s_prevSelectHeld = false;

static bool miniGameEnterOnce(const InputState &input)
{
  const bool enterOnce = (input.mgSelectHeld && !s_prevSelectHeld);
  s_prevSelectHeld = input.mgSelectHeld;
  return enterOnce || input.mgSelectOnce;
}

static UIState s_miniGameReturnUi = UIState::PET_SCREEN;
static bool s_miniGameReturnUiValid = false;
static bool tryAwardWinItem_1in4(ItemType* outType);
static const uint32_t kSurviveWinMs = 18000; // 18s survive-to-win (Flappy + Dodger)

static void mgApplyResultAndShowReward(bool won)
{
  // Apply rewards (your rule: lose => XP+5, MOOD+10 only)
  if (won)
  {
    pet.addXP(25);

    const int infReward = rollMiniGameInfReward();
    addInf(infReward);

    pet.happiness = constrain(pet.happiness + 20, 0, 100);

    ItemType rewardType = ITEM_NONE;
    const bool wonItem = tryAwardWinItem_1in4(&rewardType);

    if (wonItem) {
      const char* nm = g_app.inventory.getItemLabelForType(rewardType);
      snprintf(s_rewardMsg, sizeof(s_rewardMsg),
               "You win! XP +25  INF +%d  MOOD +20\nRandom Reward: %s +1",
               infReward,
               (nm && nm[0]) ? nm : "Item");
    } else {
      snprintf(s_rewardMsg, sizeof(s_rewardMsg),
               "You win! XP +25  INF +%d  MOOD +20",
               infReward);
    }
  }
  else
  {
    pet.addXP(5);
    pet.happiness = constrain(pet.happiness + 10, 0, 100);
    snprintf(s_rewardMsg, sizeof(s_rewardMsg),
             "You lose! XP +5  MOOD +10");
  }

  saveManagerMarkDirty();

  // Show modal + clear g_app.gameOver so we don't re-enter result logic
  s_showReward = true;
  g_app.gameOver = false;
  clearInputLatch();
}

// -----------------------------------------------------------------------------
// Mini-game global state
// -----------------------------------------------------------------------------
MiniGame currentMiniGame = MiniGame::NONE;
bool playerWon = false;

// Simple mini-game state
static bool s_resultShown = false;

static uint32_t s_mgInputLockoutUntilMs = 0;

static inline void mgBeginInputLockout(uint32_t ms) {
  s_mgInputLockoutUntilMs = millis() + ms;
}

static inline bool mgInputLockedOut() {
  return (int32_t)(millis() - s_mgInputLockoutUntilMs) < 0;
}

// -----------------------------------------------------------------------------
// FLAPPY FIREBALL (Flappy Bird clone)
// -----------------------------------------------------------------------------

static bool     s_flappyInited     = false;
static bool     s_flappyPlaying    = false;
static bool     s_flappyCrashed    = false;

static uint32_t s_flappyStartMs    = 0;

// Survive timer
static const uint32_t s_flappyWinMs = kSurviveWinMs;

static int      s_fbX              = 0;
static int      s_fbY              = 0;
static int      s_fbVY             = 0;
static uint32_t s_lastStepMs       = 0;

struct FlappyPipe {
  int x;
  int gapY;     // center of gap
  bool passed;
};

static FlappyPipe s_pipes[3];

static int flappyRandGapY(int h)
{
  const int gapH = 64;
  const int margin = 14;
  const int lo = margin + gapH / 2;
  const int hi = (h - 1) - margin - gapH / 2;
  if (hi <= lo) return h / 2;
  return lo + (int)random((long)(hi - lo + 1));
}

static void flappyResetWorld(int w, int h)
{
  s_flappyStartMs = millis();
  s_flappyPlaying = true;
  s_flappyCrashed = false;

  s_fbX  = 52;
  s_fbY  = h / 2;
  s_fbVY = 0;

  const int spacing = 140;
  const int startX  = w + 30;

  for (int i = 0; i < 3; ++i) {
    s_pipes[i].x      = startX + i * spacing;
    s_pipes[i].gapY   = flappyRandGapY(h);
    s_pipes[i].passed = false;
  }
}

void startFlappyFireball()
{
  mgPauseReset();

  // Ensure keyboard nav works (ENTER/W) even if console/text mode was left on.
  inputSetTextCapture(false);

  g_app.inMiniGame     = true;
  g_app.gameOver       = false;
  playerWon            = false;
  s_resultShown        = false;

  s_showReward         = false;
  s_rewardMsg[0]       = 0;

  s_acceptArmed        = false;
  s_gameOverMs         = 0;

  s_prevSelectHeld     = false;

  currentMiniGame      = MiniGame::FLAPPY_FIREBALL;
  miniGameSetReturnUi(g_app.uiState);
  g_app.uiState        = UIState::MINI_GAME;

  s_flappyInited       = false;
  s_lastStepMs         = millis();

  invalidateBackgroundCache();
  s_flappyStartMs      = millis();
  requestUIRedraw();
  clearInputLatch();
  mgBeginInputLockout(220);
}

static bool flappyCollides(int fbX, int fbY, int r, const FlappyPipe& p, int w, int h)
{
  const int pipeW = 26;
  const int gapH  = 64;

  const int kPipeInsetPx   = 2;
  const int kGapBonusPx    = 4;
  const int kScreenForgive = 2;

  if (fbY - r < -kScreenForgive) return true;
  if (fbY + r >= h + kScreenForgive) return true;

  int pipeL = p.x + kPipeInsetPx;
  int pipeR = p.x + pipeW - kPipeInsetPx;

  if (fbX + r < pipeL) return false;
  if (fbX - r > pipeR) return false;

  int gapTop = (p.gapY - gapH / 2) - kGapBonusPx;
  int gapBot = (p.gapY + gapH / 2) + kGapBonusPx;

  if (gapTop < 0) gapTop = 0;
  if (gapBot > h) gapBot = h;

  if (fbY - r < gapTop) return true;
  if (fbY + r > gapBot) return true;

  return false;
}

static void flappyStep(int w, int h, bool flap)
{
  const int pipeW    = 26;
  const int speedX   = 1;
  const int fbR      = 4;

  const int gravity  = 1;
  const int flapVY   = -3;

  static uint8_t s_gravCounter = 0;

  if (flap) {
    s_fbVY = flapVY;
    soundFlap();
  }

  s_gravCounter++;
  if (s_gravCounter >= 3)
  {
    s_gravCounter = 0;
    s_fbVY += gravity;
  }

  if (s_fbVY > 4) s_fbVY = 4;
  if (s_fbVY < -6) s_fbVY = -6;

  s_fbY += s_fbVY;

  int rightMost = s_pipes[0].x;
  for (int i = 1; i < 3; ++i) if (s_pipes[i].x > rightMost) rightMost = s_pipes[i].x;

  for (int i = 0; i < 3; ++i)
  {
    s_pipes[i].x -= speedX;

    if (s_pipes[i].x < -pipeW)
    {
      s_pipes[i].x      = rightMost + 140;
      s_pipes[i].gapY   = flappyRandGapY(h);
      s_pipes[i].passed = false;
      rightMost         = s_pipes[i].x;
    }
  }

  for (int i = 0; i < 3; ++i) {
    if (flappyCollides(s_fbX, s_fbY, fbR, s_pipes[i], w, h))
    {
      playerWon       = false;
      g_app.gameOver  = true;
      s_resultShown   = true;
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
  if (elapsed > pausedAccum) elapsed -= pausedAccum;
  else elapsed = 0;

  if (mgPauseIsPaused() && mgPauseStartMs() != 0) {
    uint32_t pausedSoFar = now - mgPauseStartMs();
    if (elapsed > pausedSoFar) elapsed -= pausedSoFar;
    else elapsed = 0;
  }

  return elapsed;
}

void updateFlappyFireball(const InputState &input)
{
  const bool enterOnce = miniGameEnterOnce(input);

  if (s_showReward)
  {
    if (enterOnce) {
      exitMiniGameToReturnUi(true);
    }
    return;
  }

  if (g_app.gameOver)
  {
    if (enterOnce)
    {
      if (currentMiniGame == MiniGame::RESURRECTION)
      {
        currentMiniGame = MiniGame::NONE;
        g_app.inMiniGame = false;
        g_app.gameOver = false;

        onResurrectionMiniGameResult(playerWon);

        g_app.uiState = UIState::PET_SCREEN;
        requestUIRedraw();
        clearInputLatch();
        return;
      }

      mgApplyResultAndShowReward(playerWon);
      mgBeginInputLockout(220);
    }
    return;
  }

  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  if (!s_flappyInited) {
    s_flappyInited = true;
    flappyResetWorld(gW, gH);
    s_lastStepMs = millis();
  }

  const uint32_t now = millis();

  if (mgPauseIsPaused()) {
    s_lastStepMs = now;
    return;
  }

  if (mgPauseJustResumedConsume()) {
    s_lastStepMs = now;
  }

  const uint32_t aliveMs = flappyAliveMsNow(now);
  if (aliveMs >= s_flappyWinMs) {
    playerWon       = true;
    g_app.gameOver  = true;
    s_resultShown   = true;
    s_flappyPlaying = false;
    soundConfirm();
    return;
  }

  const bool flap =
      input.mgSelectOnce ||
      input.mgSelectHeld ||
      input.mgUpOnce ||
      input.mgUpHeld;

  const uint32_t stepMs = 16;

  if ((int32_t)(now - s_lastStepMs) >= 0)
  {
    bool flapUsed = false;

    while ((int32_t)(now - s_lastStepMs) >= 0)
    {
      const bool flapThisStep = (flap && !flapUsed);
      flappyStep(gW, gH, flapThisStep);
      if (flapThisStep) flapUsed = true;

      s_lastStepMs += stepMs;
      if (g_app.gameOver) break;
    }
  }
}

static void drawRewardModal(int gW, int gH)
{
  spr.setTextDatum(CC_DATUM);

  const char* nl = strchr(s_rewardMsg, '\n');
  if (nl)
  {
    char line1[64];
    size_t len = (size_t)(nl - s_rewardMsg);
    if (len > sizeof(line1) - 1) len = sizeof(line1) - 1;

    memcpy(line1, s_rewardMsg, len);
    line1[len] = 0;

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString(line1, gW/2, gH/2 - 12, 2);

    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
    spr.drawCentreString(nl + 1, gW/2, gH/2 + 6, 2);
  }
  else
  {
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString(s_rewardMsg, gW/2, gH/2 - 6, 2);
  }

  spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  spr.drawCentreString("Press ENTER", gW/2, gH/2 + 28, 2);
}

void drawFlappyFireball()
{
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

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
    spr.drawCentreString(playerWon ? "YOU WIN!" : "YOU LOSE!", gW/2, gH/2 - 10, 4);

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW/2, gH/2 + 22, 2);
    return;
  }

  const int pipeW = 26;
  const int gapH  = 64;

  for (int i = 0; i < 3; ++i)
  {
    const int x = s_pipes[i].x;
    const int gapTop = s_pipes[i].gapY - gapH / 2;
    const int gapBot = s_pipes[i].gapY + gapH / 2;

    spr.fillRect(x, 0, pipeW, gapTop, TFT_DARKGREY);
    spr.fillRect(x, gapBot, pipeW, gH - gapBot, TFT_DARKGREY);
  }

  spr.fillCircle(s_fbX, s_fbY, 5, TFT_ORANGE);
  spr.drawCircle(s_fbX, s_fbY, 5, TFT_RED);

  {
    const uint32_t now = millis();
    const uint32_t aliveMs = flappyAliveMsNow(now);
    const uint32_t winMs   = s_flappyWinMs;

    int barW = gW - 20;
    int barX = 10;
    int barY = 6;
    int barH = 6;

    spr.drawRect(barX, barY, barW, barH, TFT_DARKGREY);

    int fill = (int)((aliveMs * (uint32_t)(barW - 2)) / winMs);
    if (fill < 0) fill = 0;
    if (fill > barW - 2) fill = barW - 2;

    spr.fillRect(barX + 1, barY + 1, fill, barH - 2, TFT_YELLOW);
  }
}

// -----------------------------------------------------------------------------
// Random INF reward (WIN only)
// -----------------------------------------------------------------------------
static int rollMiniGameInfReward()
{
  const int r = (int)random(100);

  if (r < 50) return 25;
  if (r < 80) return 50;
  if (r < 95) return 75;
  return 100;
}

// -----------------------------------------------------------------------------
// Mini-game reward (WIN only): 25% chance to win ONE random item
// -----------------------------------------------------------------------------
static bool tryAwardWinItem_1in4(ItemType* outType) {
  if (outType) *outType = ITEM_NONE;

  if (random(4) != 0) return false;

  static const ItemType kRewards[] = {
    ITEM_SOUL_FOOD,
    ITEM_CURSED_RELIC,
    ITEM_DEMON_BONE,
    ITEM_RITUAL_CHALK
  };

  const int n = (int)(sizeof(kRewards) / sizeof(kRewards[0]));
  const ItemType t = kRewards[(int)random((long)n)];

  g_app.inventory.addItem(t, 1);
  if (outType) *outType = t;
  return true;
}

void miniGameExitToReturnUi(bool beginLockout) {
  s_showReward = false;
  s_rewardMsg[0] = 0;

  g_app.inMiniGame   = false;
  g_app.gameOver     = false;
  playerWon          = false;
  currentMiniGame    = MiniGame::NONE;

  g_app.uiState = miniGameGetReturnUiOrDefault(UIState::PET_SCREEN);
  miniGameClearReturnUi();

  mgPauseReset();
  clearInputLatch();
  requestUIRedraw();

  if (beginLockout) {
    mgBeginInputLockout(220);
  }
}

// Back-compat: older call sites used this name.
static void exitMiniGameToReturnUi(bool beginLockout)
{
  miniGameExitToReturnUi(beginLockout);
}

static void mgSyncGameTimebases(uint32_t now);

// -----------------------------------------------------------------------------
// Universal mini-game update/draw dispatch + pause overlay
// -----------------------------------------------------------------------------

void updateMiniGame(const InputState& input)
{
  if (currentMiniGame == MiniGame::NONE) return;

  const uint32_t now = millis();

  const bool activePlay =
      g_app.inMiniGame &&
      !s_showReward &&
      !g_app.gameOver;

  if (!activePlay) {
    mgPauseForceOffNoStick();
  } else {
    if (!mgInputLockedOut()) {
      const uint8_t p = mgPauseHandle(input);

      mgPauseUpdateClocks(now);

      if (p == MGPAUSE_EXIT) {
        exitMiniGameToReturnUi(true);
        return;
      }

      if (p == MGPAUSE_CONSUME) {
        if (mgPauseIsPaused()) {
          mgSyncGameTimebases(now);
        }
        return;
      }
    } else {
      mgPauseUpdateClocks(now);
    }

    if (mgPauseIsPaused()) {
      mgSyncGameTimebases(now);
      return;
    }
  }

  switch (currentMiniGame)
  {
    case MiniGame::FLAPPY_FIREBALL:   updateFlappyFireball(input);   break;
    case MiniGame::CROSSY_ROAD:       updateCrossyRoad(input);       break;
    case MiniGame::INFERNAL_DODGER:   updateInfernalDodger(input);   break;
    case MiniGame::RESURRECTION:      updateResurrectionRun(input);  break;
    default: break;
  }
}

void drawMiniGame()
{
  // Keep pause clocks updated so the overlay can draw reliably.
  const uint32_t now = millis();
  mgPauseUpdateClocks(now);

  switch (currentMiniGame)
  {
    case MiniGame::FLAPPY_FIREBALL:   drawFlappyFireball();   break;
    case MiniGame::CROSSY_ROAD:       drawCrossyRoad();       break;
    case MiniGame::INFERNAL_DODGER:   drawInfernalDodger();   break;
    case MiniGame::RESURRECTION:      drawResurrectionRun();  break;
    default: break;
  }

  if (mgPauseIsPaused())
  {
    mgDrawPauseOverlay();
  }
}

// -----------------------------------------------------------------------------
// Resurrection Run (side-scroller runner)
// -----------------------------------------------------------------------------

static bool     rr_active = false;
static bool     rr_gameOver = false;
static bool     rr_won = false;
static bool     rr_ducking = false;

static float    rr_y = 0.0f;
static float    rr_vy = 0.0f;
static bool     rr_onGround = true;

static int      rr_distance = 0;
static uint32_t rr_lastMs = 0;

struct RRObs {
  int x;
  int y;
  int w;
  int h;
  bool active;
};

static RRObs rr_obs[8];
static int   rr_courseLen = 2600;

struct RRSpawn {
  int triggerDist;
  uint8_t type;
  uint8_t param;
};

static const uint8_t RR_SPIKE    = 0;
static const uint8_t RR_LOW_FIRE = 1;

static const RRSpawn rr_script[] = {
  {  260, RR_SPIKE,    0 },
  {  520, RR_SPIKE,    0 },
  {  780, RR_LOW_FIRE, 0 },
  { 1020, RR_SPIKE,    0 },
  { 1280, RR_LOW_FIRE, 0 },
  { 1520, RR_SPIKE,    0 },
  { 1780, RR_SPIKE,    0 },
  { 2040, RR_LOW_FIRE, 0 },
  { 2280, RR_SPIKE,    0 },
  { 2540, RR_LOW_FIRE, 0 },
};

static const int rr_scriptCount = (int)(sizeof(rr_script) / sizeof(rr_script[0]));
static int rr_nextSpawn = 0;

static void rrSpawnObstacle(uint8_t type) {
  const int w = (screenW > 0) ? screenW : 240;
const int h = (screenH > 0) ? screenH : 135;
  int slot = -1;
  for (int i = 0; i < (int)(sizeof(rr_obs) / sizeof(rr_obs[0])); i++) {
    if (!rr_obs[i].active) { slot = i; break; }
  }
  if (slot < 0) return;

  const int spawnScreenLead = 60;
  const int spawnWorldX = rr_distance + w + spawnScreenLead;

  const int groundY = h - 18;

  if (type == RR_SPIKE) {
    rr_obs[slot].x = spawnWorldX;
    rr_obs[slot].y = groundY - 14;
    rr_obs[slot].w = 16;
    rr_obs[slot].h = 14;
    rr_obs[slot].active = true;  } else {
      rr_obs[slot].x = spawnWorldX;
      rr_obs[slot].y = groundY - 32;
      rr_obs[slot].w = 18;
      rr_obs[slot].h = 10;
      rr_obs[slot].active = true;  }
}

static void rrResetObstacles() {
  for (auto &o : rr_obs) o = {0,0,0,0,false};
}

static bool rrAabb(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh) {
  return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
}

void startResurrectionRun() {
  mgPauseReset();
  inputSetTextCapture(false);

  // Make Resurrection Run behave like all other mini-games (state + flags)
  g_app.inMiniGame   = true;
  g_app.gameOver     = false;
  playerWon          = false;
  s_resultShown      = false;

  currentMiniGame    = MiniGame::RESURRECTION;
  miniGameSetReturnUi(g_app.uiState);
  g_app.uiState      = UIState::MINI_GAME;

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

void updateResurrectionRun(const InputState& input) {

  if (rr_gameOver) {
    const bool enterOnce = miniGameEnterOnce(input);

    if (enterOnce) {
      rr_active   = false;
      rr_gameOver = false;

      playerWon = rr_won;

      currentMiniGame = MiniGame::NONE;
      g_app.inMiniGame = false;

      onResurrectionMiniGameResult(playerWon);

      miniGameClearReturnUi();

      mgPauseReset();
      clearInputLatch();
      requestUIRedraw();
      mgBeginInputLockout(220);
    }
    return;
  }

  const uint32_t now = millis();
  uint32_t dtMs = now - rr_lastMs;
  rr_lastMs = now;
  if (dtMs > 40) dtMs = 40;
  const float dt = dtMs / 1000.0f;

  rr_ducking = (input.mgDownHeld || input.mgSpaceHeld);

  const bool jumpOnce = input.mgSelectOnce || input.mgUpOnce;
  const bool jumpHeld = input.mgSelectHeld || input.mgUpHeld;

  if (jumpOnce && rr_onGround) {
    rr_vy = -220.0f;
    rr_onGround = false;
  }

  const float gravity = (jumpHeld && rr_vy < 0.0f) ? 520.0f : 720.0f;

  rr_vy += gravity * dt;
  rr_y  += rr_vy * dt;

  if (rr_y >= 0.0f) {
    rr_y = 0.0f;
    rr_vy = 0.0f;
    rr_onGround = true;
  }

  const int speed = 290;
  rr_distance += (int)(speed * dt);

  while (rr_nextSpawn < rr_scriptCount && rr_distance >= rr_script[rr_nextSpawn].triggerDist) {
    rrSpawnObstacle(rr_script[rr_nextSpawn].type);
    rr_nextSpawn++;
  }

  if (rr_distance >= rr_courseLen) {
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
  if (rr_ducking) py = groundY - ph + (int)rr_y;

  for (auto &o : rr_obs) {
    if (!o.active) continue;

    int ox = o.x - rr_distance;
    int oy = o.y;
    if (rrAabb(px, py, pw, ph, ox, oy, o.w, o.h)) {
      rr_gameOver = true;
      rr_won = false;
      return;
    }

    if (ox < -40) o.active = false;
  }
}

void drawResurrectionRun() {
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  spr.fillSprite(TFT_BLACK);

  if (rr_gameOver)
  {
    spr.setTextDatum(CC_DATUM);
    spr.setTextColor(rr_won ? TFT_GREEN : TFT_RED, TFT_BLACK);
    spr.drawCentreString(rr_won ? "RESURRECTED!" : "FALLEN...", gW/2, gH/2 - 10, 4);

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW/2, gH/2 + 22, 2);
    return;
  }

  const int groundY = gH - 18;
  spr.drawLine(0, groundY, gW, groundY, TFT_DARKGREY);

  int px = 48;
  int py = groundY - 18 + (int)rr_y;
  int pw = 16;
  int ph = rr_ducking ? 10 : 16;
  if (rr_ducking) py = groundY - ph + (int)rr_y;
  spr.fillRect(px, py, pw, ph, TFT_GREEN);

  for (auto &o : rr_obs) {
    if (!o.active) continue;
    int ox = o.x - rr_distance;
    if (ox < -40 || ox > gW + 40) continue;
    spr.fillRect(ox, o.y, o.w, o.h, TFT_RED);
  }

  int barW = gW - 20;
  int barX = 10;
  int barY = 6;
  spr.drawRect(barX, barY, barW, 6, TFT_DARKGREY);
  int fill = (rr_distance * (barW - 2)) / rr_courseLen;
  if (fill < 0) fill = 0;
  if (fill > barW - 2) fill = barW - 2;
  spr.fillRect(barX + 1, barY + 1, fill, 4, TFT_YELLOW);
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
static const int kCrossyLastTrafficRow  = kCrossyRows - 2;

struct CrossyLane {
  int8_t  dir;
  uint8_t speed;
  uint8_t carLen;
  uint16_t gapPx;
  int32_t offsetPx;
};

static CrossyLane s_crossyLanes[kCrossyRows];

static int s_crossyPx = 0;
static int s_crossyPy = 0;

static bool     s_crossyInited = false;
static uint32_t s_crossyLastLaneMs = 0;

static inline int crossyClamp(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void crossyInitLanes()
{
  memset(s_crossyLanes, 0, sizeof(s_crossyLanes));

  for (int r = kCrossyFirstTrafficRow; r <= kCrossyLastTrafficRow; ++r) {
    CrossyLane& L = s_crossyLanes[r];
    L.dir    = (r % 2 == 0) ? +1 : -1;
    L.speed  = (uint8_t)(1 + (r % 2));
    L.carLen = (uint8_t)(1 + (r % 2));
    L.gapPx  = (uint16_t)(kCrossyTileW * (4 + (r % 3)));
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

  g_app.inMiniGame    = true;
  g_app.gameOver      = false;
  playerWon           = false;
  s_resultShown       = false;

  s_showReward        = false;
  s_rewardMsg[0]      = 0;

  currentMiniGame     = MiniGame::CROSSY_ROAD;
  miniGameSetReturnUi(g_app.uiState);
  g_app.uiState       = UIState::MINI_GAME;

  s_crossyInited      = true;
  crossyReset();

  invalidateBackgroundCache();
  requestUIRedraw();
  clearInputLatch();
  mgBeginInputLockout(220);
}

static void crossyStepLanes(uint32_t now)
{
  const uint32_t kLaneStepMs = 50;
  if ((uint32_t)(now - s_crossyLastLaneMs) < kLaneStepMs) return;
  s_crossyLastLaneMs = now;

  for (int r = kCrossyFirstTrafficRow; r <= kCrossyLastTrafficRow; ++r) {
    CrossyLane& L = s_crossyLanes[r];
    L.offsetPx += (int32_t)L.speed * (L.dir > 0 ? 1 : -1);
  }
}

static bool crossyHitCar()
{
  if (s_crossyPy < kCrossyFirstTrafficRow || s_crossyPy > kCrossyLastTrafficRow) return false;

  const CrossyLane& L = s_crossyLanes[s_crossyPy];

  const int carLenPx = (int)L.carLen * kCrossyTileW;
  const int periodPx = carLenPx + (int)L.gapPx;

  const int laneW = kCrossyCols * kCrossyTileW;

  const int playerX0 = s_crossyPx * kCrossyTileW;
  const int playerX1 = playerX0 + (kCrossyTileW - 1);

  for (int testX = playerX0; testX <= playerX1; testX += (kCrossyTileW / 2)) {
    int shifted = testX + (int)L.offsetPx;
    int m = shifted % periodPx;
    if (m < 0) m += periodPx;

    if (testX < 0 || testX >= laneW) continue;

    if (m >= 0 && m < carLenPx) return true;
  }

  return false;
}

void updateCrossyRoad(const InputState& input)
{
  const bool enterOnce = miniGameEnterOnce(input);

  if (s_showReward)
  {
    if (enterOnce) {
      exitMiniGameToReturnUi(true);
    }
    return;
  }

  if (g_app.gameOver)
  {
    if (enterOnce)
    {
      mgApplyResultAndShowReward(playerWon);
      mgBeginInputLockout(220);
    }
    return;
  }

  if (!s_crossyInited) {
    s_crossyInited = true;
    crossyReset();
  }

  crossyStepLanes(millis());

  int dx = 0, dy = 0;

  if (input.mgLeftOnce)  dx = -1;
  if (input.mgRightOnce) dx = +1;
  if (input.mgUpOnce)    dy = -1;
  if (input.mgDownOnce)  dy = +1;

  if (input.encoderDelta < 0) dy = -1;
  if (input.encoderDelta > 0) dy = +1;

  if (dx || dy) {
    s_crossyPx = crossyClamp(s_crossyPx + dx, 0, kCrossyCols - 1);
    s_crossyPy = crossyClamp(s_crossyPy + dy, 0, kCrossyRows - 1);
    playBeep();
  }

  if (s_crossyPy == 0) {
    playerWon = true;
    g_app.gameOver = true;
    s_resultShown = true;
    soundConfirm();
    return;
  }

  if (crossyHitCar()) {
    playerWon = false;
    g_app.gameOver = true;
    s_resultShown = true;
    soundError();
    return;
  }
}

void drawCrossyRoad()
{
  if (!s_crossyInited) {
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
    spr.drawCentreString(playerWon ? "YOU WIN!" : "YOU LOSE!", gW/2, gH/2 - 10, 4);

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW/2, gH/2 + 22, 2);
    return;
  }

  for (int y = 0; y < kCrossyRows; ++y) {
    const int ry = kCrossyOriginY + y * kCrossyTileH;

    uint16_t col = TFT_NAVY;
    if (y == 0) col = TFT_DARKGREEN;
    else if (y == kCrossyRows - 1) col = TFT_DARKGREY;

    spr.fillRect(kCrossyOriginX, ry, kCrossyCols * kCrossyTileW, kCrossyTileH, col);
  }

  for (int r = kCrossyFirstTrafficRow; r <= kCrossyLastTrafficRow; ++r) {
    const CrossyLane& L = s_crossyLanes[r];

    const int carLenPx = (int)L.carLen * kCrossyTileW;
    const int periodPx = carLenPx + (int)L.gapPx;
    const int laneW    = kCrossyCols * kCrossyTileW;

    if (carLenPx <= 0 || periodPx <= 0) continue;

    const int y = kCrossyOriginY + r * kCrossyTileH;

    int offset = (int)(L.offsetPx % periodPx);
    if (offset < 0) offset += periodPx;

    for (int x0 = -periodPx * 2; x0 < laneW + periodPx * 2; x0 += periodPx) {
      const int x = x0 - offset;
      const int drawX = kCrossyOriginX + x;

      if (drawX + carLenPx < kCrossyOriginX) continue;
      if (drawX > kCrossyOriginX + laneW) continue;

      spr.fillRect(drawX, y + 2, carLenPx - 2, kCrossyTileH - 4, TFT_RED);
      spr.fillRect(drawX + 2, y + 4, carLenPx - 6, 2, TFT_WHITE);
    }
  }

  const int fx = kCrossyOriginX + s_crossyPx * kCrossyTileW;
  const int fy = kCrossyOriginY + s_crossyPy * kCrossyTileH;
  spr.fillRect(fx + 4, fy + 3, kCrossyTileW - 8, kCrossyTileH - 6, TFT_GREEN);
}

// -----------------------------------------------------------------------------
// INFERNAL DODGER
// -----------------------------------------------------------------------------

struct DodgerBall {
  int16_t x;
  int16_t y;
  int16_t vy;
  uint8_t r;
  bool    active;
};

static bool     s_dodgerInited = false;
static uint32_t s_dodgerLastStepMs = 0;
static uint32_t s_dodgerStartMs = 0;
static uint32_t s_dodgerSpawnAccMs = 0;

static int16_t  s_dodgerPx = 0;
static int16_t  s_dodgerPy = 0;
static int16_t  s_dodgerSpeed = 3;
static float    s_dodgerPxF = 0.0f;
static uint32_t s_dodgerMoveLastMs = 0;

static int8_t   s_dodgerMoveDir = 0;
static uint32_t s_dodgerDirHoldMs = 0;

static DodgerBall s_dodgerBalls[8];

static void dodgerReset() {
  const int gW = (screenW > 0) ? screenW : 240;
  const int gH = (screenH > 0) ? screenH : 135;

  s_dodgerPx = gW / 2;
  s_dodgerPxF = (float)s_dodgerPx;
  s_dodgerMoveLastMs = millis();
  s_dodgerPy = gH - 14;
  s_dodgerSpeed = 3;

  for (auto &b : s_dodgerBalls) {
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

static void dodgerSpawnOne(int difficulty) {
  const int gW = (screenW > 0) ? screenW : 240;

  int slot = -1;
  for (int i = 0; i < (int)(sizeof(s_dodgerBalls)/sizeof(s_dodgerBalls[0])); ++i) {
    if (!s_dodgerBalls[i].active) { slot = i; break; }
  }
  if (slot < 0) return;

  DodgerBall &b = s_dodgerBalls[slot];

  const int margin = 6;
  b.r = (uint8_t)(3 + (difficulty % 3));
  b.x = (int16_t)random((long)(margin), (long)(gW - margin));
  b.y = (int16_t)(- (int)(10 + random(40)));

  b.vy = (int16_t)(2 + (difficulty / 5));
  if (b.vy > 7) b.vy = 7;

  b.active = true;
}

static inline bool dodgerHit(int ax, int ay, int ar, int bx, int by, int br)
{
  const int dx = ax - bx;
  const int dy = ay - by;
  const int rr = ar + br;
  return (dx*dx + dy*dy) <= (rr*rr);
}

void startInfernalDodger()
{
  inputSetTextCapture(false);
  mgPauseReset();

  g_app.inMiniGame     = true;
  g_app.gameOver       = false;
  playerWon            = false;
  s_resultShown        = false;

  s_showReward         = false;
  s_rewardMsg[0]       = 0;

  s_prevSelectHeld     = false;

  currentMiniGame      = MiniGame::INFERNAL_DODGER;
  miniGameSetReturnUi(g_app.uiState);
  g_app.uiState        = UIState::MINI_GAME;

  s_dodgerInited       = false;

  invalidateBackgroundCache();
  requestUIRedraw();
  clearInputLatch();
  mgBeginInputLockout(220);
}

void updateInfernalDodger(const InputState& input)
{
  const bool enterOnce = miniGameEnterOnce(input);

  if (s_showReward)
  {
    if (enterOnce) {
      exitMiniGameToReturnUi(true);
    }
    return;
  }

  if (g_app.gameOver)
  {
    if (enterOnce)
    {
      mgApplyResultAndShowReward(playerWon);
      mgBeginInputLockout(220);
    }
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
  const uint32_t aliveMs = now - s_dodgerStartMs;
  const int difficulty = (int)(aliveMs / 3000);

  const uint32_t kWinMs = kSurviveWinMs;
  if (aliveMs >= kWinMs)
  {
    playerWon = true;
    g_app.gameOver  = true;
    s_resultShown = true;
    soundConfirm();
    return;
  }

  const bool leftHeld  = input.mgLeftHeld;
  const bool rightHeld = input.mgRightHeld;

  if (input.mgLeftOnce)  { s_dodgerMoveDir = -1; s_dodgerDirHoldMs = now + 140; }
  if (input.mgRightOnce) { s_dodgerMoveDir = +1; s_dodgerDirHoldMs = now + 140; }

  if (leftHeld && !rightHeld)  { s_dodgerMoveDir = -1; s_dodgerDirHoldMs = now + 140; }
  if (rightHeld && !leftHeld)  { s_dodgerMoveDir = +1; s_dodgerDirHoldMs = now + 140; }

  if (!leftHeld && !rightHeld) {
    if ((int32_t)(now - s_dodgerDirHoldMs) >= 0) {
      s_dodgerMoveDir = 0;
    }
  }

  if (input.encoderDelta < 0) { s_dodgerMoveDir = -1; s_dodgerDirHoldMs = now + 140; }
  if (input.encoderDelta > 0) { s_dodgerMoveDir = +1; s_dodgerDirHoldMs = now + 140; }

  uint32_t mvDtMs = now - s_dodgerMoveLastMs;
  s_dodgerMoveLastMs = now;
  if (mvDtMs > 40) mvDtMs = 40;

  float pxPerSec = 120.0f + (float)(difficulty * 6);
  if (pxPerSec > 170.0f) pxPerSec = 170.0f;

  const float dt = (float)mvDtMs / 1000.0f;
  s_dodgerPxF += (float)s_dodgerMoveDir * pxPerSec * dt;

  const float marginF = 6.0f;
  if (s_dodgerPxF < marginF) s_dodgerPxF = marginF;
  if (s_dodgerPxF > (float)gW - marginF) s_dodgerPxF = (float)gW - marginF;

  s_dodgerPx = (int16_t)(s_dodgerPxF + 0.5f);

  const int margin = 6;
  if (s_dodgerPx < margin) s_dodgerPx = margin;
  if (s_dodgerPx > gW - margin) s_dodgerPx = gW - margin;

  const uint32_t stepMs = 16;
  while ((int32_t)(now - s_dodgerLastStepMs) >= 0)
  {
    int spawnEveryMs = 520 - difficulty * 24;
    if (spawnEveryMs < 220) spawnEveryMs = 220;

    s_dodgerSpawnAccMs += stepMs;
    if (s_dodgerSpawnAccMs >= (uint32_t)spawnEveryMs) {
      s_dodgerSpawnAccMs = 0;
      dodgerSpawnOne(difficulty);
    }

    for (auto &b : s_dodgerBalls)
    {
      if (!b.active) continue;

      b.y += b.vy;

      if (b.y > gH + 12) {
        b.active = false;
        continue;
      }

      const int pr = 6;
      if (dodgerHit((int)s_dodgerPx, (int)s_dodgerPy, pr, (int)b.x, (int)b.y, (int)b.r))
      {
        playerWon = false;
        g_app.gameOver  = true;
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

  spr.fillSprite(TFT_BLACK);

  {
    const uint32_t aliveMs = millis() - s_dodgerStartMs;
    const uint32_t kWinMs = kSurviveWinMs;
    int barW = gW - 20;
    int barX = 10;
    int barY = 6;

    spr.drawRect(barX, barY, barW, 6, TFT_DARKGREY);

    int fill = (int)((aliveMs * (uint32_t)(barW - 2)) / kWinMs);
    if (fill < 0) fill = 0;
    if (fill > barW - 2) fill = barW - 2;

    spr.fillRect(barX + 1, barY + 1, fill, 4, TFT_YELLOW);
  }

  if (s_showReward)
  {
    drawRewardModal(gW, gH);
    return;
  }

  if (g_app.gameOver)
  {
    spr.setTextDatum(CC_DATUM);
    spr.setTextColor(playerWon ? TFT_GREEN : TFT_RED, TFT_BLACK);
    spr.drawCentreString(playerWon ? "YOU WIN!" : "YOU LOSE!", gW/2, gH/2 - 10, 4);

    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawCentreString("Press ENTER", gW/2, gH/2 + 22, 2);
    return;
  }

  for (auto &b : s_dodgerBalls)
  {
    if (!b.active) continue;
    spr.fillCircle(b.x, b.y, b.r, TFT_ORANGE);
    spr.drawCircle(b.x, b.y, b.r, TFT_RED);
  }

  spr.fillCircle(s_dodgerPx, s_dodgerPy, 6, TFT_GREEN);
  spr.drawCircle(s_dodgerPx, s_dodgerPy, 6, TFT_DARKGREEN);
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