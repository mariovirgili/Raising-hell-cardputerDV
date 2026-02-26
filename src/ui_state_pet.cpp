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

// This is the exact function body that used to live in menu_actions.cpp.
static void handlePetScreen_local(const InputState &input)
{
  // Pet screen is NOT a text-entry surface. If something left text-capture on
  // (console exit regressions), Enter/arrows can stop working here.
  inputSetTextCapture(false);

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
      clearInputLatch();
      return;
    }

    const bool confirmOnce =
        input.selectOnce || input.encoderPressOnce || input.mgSelectOnce;

    if (confirmOnce)
    {
      // Prevent the confirm press from leaking into any follow-up UI.
      inputForceClear();

      feedUseSelected();
      clearInputLatch();
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
      clearInputLatch();
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
        clearInputLatch();
        return;
      }

      pet.energy = constrain(pet.energy - 10, 0, 100);
      saveManagerMarkDirty();

      // IMPORTANT:
      // Clear any held/latched key state so the Enter that launches the game
      // doesn't immediately act inside the mini-game or wedge nav input.
      inputForceClear();
      clearInputLatch();

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

      // start*() already clears latches too, but keeping this is harmless and
      // helps if a start routine changes in the future.
      clearInputLatch();
      return;
    }

    return;
  }
}

void uiPetScreenHandle(InputState &input)
{
  handlePetScreen_local(input);
}