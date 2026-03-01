#include "ui_state_pet.h"

#include <Arduino.h>

#include "app_state.h"
#include "display.h"
#include "feed_actions.h"
#include "feed_menu_actions.h"
#include "graphics.h"
#include "input.h"
#include "mini_games.h" // startFlappyFireball(), startInfernalDodger(), startCrossyRoad()
#include "pet.h"
#include "save_manager.h"
#include "sound.h"
#include "ui_menu_state.h" // feedMenuIndex, playMenuIndex
#include "ui_actions.h"    // uiActionSwallowEdges, uiActionDrainKb
#include "ui_play_menu.h"
#include "ui_feed_menu.h"

// PET/STATS/FEED/PLAY all ride UIState::PET_SCREEN.
//
// IMPORTANT:
// Do NOT call clearInputLatch() in this handler.
// clearInputLatch() rewrites the ESC latch based on a "held ESC" probe,
// which can get stuck and prevent escOnce from ever firing on PET_SCREEN tabs.
// Use uiActionSwallowEdges + uiActionDrainKb to swallow without touching key latches.

static inline void swallowThisFrame(InputState& in)
{
  uiActionSwallowEdges(in);
  uiActionDrainKb(in);
}

static void handlePetScreen_local(InputState &input)
{
  // Pet screen is NOT a text-entry surface. If something left text-capture on,
  // Enter/arrows can stop working here.
  if (g_textCaptureMode)
  {
    inputSetTextCapture(false);
  }

  if (g_app.currentTab == Tab::TAB_FEED)
  {
    const int kFeedItems = uiFeedMenuCount();

    int mv = 0;
    if (input.leftOnce || input.upOnce || (input.encoderDelta < 0))
      mv = -1;
    if (input.rightOnce || input.downOnce || (input.encoderDelta > 0))
      mv = +1;

    if (mv != 0)
    {
      feedMenuIndex += mv;
      while (feedMenuIndex < 0)
        feedMenuIndex += kFeedItems;
      feedMenuIndex %= kFeedItems;

      requestUIRedraw();
      playBeep();

      swallowThisFrame(input);
      return;
    }

    const bool confirmOnce =
        input.selectOnce || input.encoderPressOnce || input.mgSelectOnce;

    if (confirmOnce)
    {
      inputForceClear();

      uiFeedMenuActivate(feedMenuIndex, input);

      swallowThisFrame(input);
    }
    return;
  }

  if (g_app.currentTab == Tab::TAB_PLAY)
  {
    const int kPlayItems = uiPlayMenuCount();

    int mv = 0;
    if (input.leftOnce || input.upOnce || (input.encoderDelta < 0))
      mv = -1;
    if (input.rightOnce || input.downOnce || (input.encoderDelta > 0))
      mv = +1;

    if (mv != 0)
    {
      playMenuIndex += mv;
      while (playMenuIndex < 0)
        playMenuIndex += kPlayItems;
      playMenuIndex %= kPlayItems;

      requestUIRedraw();
      playBeep();

      swallowThisFrame(input);
      return;
    }

    const bool confirmOnce =
        input.selectOnce || input.encoderPressOnce || input.mgSelectOnce;

    if (confirmOnce)
    {
      // Cost to play: 10 energy (regular Play tab only)
      if (pet.energy < 10)
      {
        ui_showMessage("Too tired!");
        soundError();
        swallowThisFrame(input);
        return;
      }

      pet.energy = constrain(pet.energy - 10, 0, 100);
      saveManagerMarkDirty();

      // Clear any queued/held input so Enter doesn't immediately act in the game.
      inputForceClear();

      // Launch selected game
      uiPlayMenuActivate(playMenuIndex, input);

      // Swallow leftover edges for this frame (no latch reset).
      swallowThisFrame(input);
      return;
    }

    return;
  }

  // PET / STATS tabs: passive; no special input here.
}

void uiPetScreenHandle(InputState &input)
{
  handlePetScreen_local(input);
}