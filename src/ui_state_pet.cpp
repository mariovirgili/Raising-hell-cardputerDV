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
    int mv = 0;
    if (input.leftOnce || input.upOnce || (input.encoderDelta < 0))
      mv = -1;
    if (input.rightOnce || input.downOnce || (input.encoderDelta > 0))
      mv = +1;

    if (mv != 0)
    {
      feedMenuIndex += mv;
      if (feedMenuIndex < 0)
        feedMenuIndex = 1;
      if (feedMenuIndex > 1)
        feedMenuIndex = 0;

      requestUIRedraw();
      playBeep();

      // Swallow this navigation so it doesn't cascade, but keep ESC latch intact.
      swallowThisFrame(input);
      return;
    }

    const bool confirmOnce =
        input.selectOnce || input.encoderPressOnce || input.mgSelectOnce;

    if (confirmOnce)
    {
      // Prevent the confirm press from leaking into any follow-up UI.
      inputForceClear();

      feedUseSelected();

      // Swallow any remaining edges/keys this frame (no latch reset).
      swallowThisFrame(input);
    }
    return;
  }

  if (g_app.currentTab == Tab::TAB_PLAY)
  {
    // Play tab games (keep in sync with drawPlayTabMock() labels)
    static const int kPlayItems = 3;

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
      if (playMenuIndex == 0)
      {
        startFlappyFireball();
      }
      else if (playMenuIndex == 1)
      {
        startInfernalDodger();
      }
      else
      {
        startCrossyRoad();
      }

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