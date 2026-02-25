#include "ui_state_pet.h"

#include <Arduino.h>

#include "app_state.h"
#include "display.h"
#include "graphics.h"
#include "input.h"
#include "feed_menu_actions.h"
#include "mini_games.h"     // startFlappyFireball(), startInfernalDodger(), startCrossyRoad()
#include "pet.h"
#include "save_manager.h"
#include "sound.h"
#include "ui_menu_state.h"  // feedMenuIndex, playMenuIndex

// This is the exact function body that used to live in menu_actions.cpp.
static void handlePetScreen_local(const InputState &input) {
  if (g_app.currentTab == Tab::TAB_FEED) {
    int mv = 0;
    if (input.leftOnce || input.upOnce || (input.encoderDelta < 0)) mv = -1;
    if (input.rightOnce || input.downOnce || (input.encoderDelta > 0)) mv = +1;

    if (mv != 0) {
      feedMenuIndex += mv;
      if (feedMenuIndex < 0) feedMenuIndex = 1;
      if (feedMenuIndex > 1) feedMenuIndex = 0;

      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      feedUseSelected();
      clearInputLatch();
    }
    return;
  }

  if (g_app.currentTab == Tab::TAB_PLAY) {
    // Play tab games (keep in sync with drawPlayTabMock() labels)
    static const int kPlayItems = 3;

    int mv = 0;
    if (input.leftOnce || input.upOnce || (input.encoderDelta < 0)) mv = -1;
    if (input.rightOnce || input.downOnce || (input.encoderDelta > 0)) mv = +1;

    if (mv != 0) {
      playMenuIndex += mv;
      while (playMenuIndex < 0) playMenuIndex += kPlayItems;
      playMenuIndex %= kPlayItems;

      requestUIRedraw();
      playBeep();
      clearInputLatch();
      return;
    }

    if (input.selectOnce || input.encoderPressOnce) {
      // Cost to play: 10 energy (regular Play tab only)
      if (pet.energy < 10) {
        ui_showMessage("Too tired!");
        soundError();
        clearInputLatch();
        return;
      }

      pet.energy = constrain(pet.energy - 10, 0, 100);
      saveManagerMarkDirty();

      // Launch selected game
      if (playMenuIndex == 0) {
        startFlappyFireball();
      } else if (playMenuIndex == 1) {
        startInfernalDodger();
      } else {
        startCrossyRoad();
      }

      clearInputLatch();
      return;
    }

    return;
  }
}

void uiPetScreenHandle(InputState& input) {
  handlePetScreen_local(input);
}