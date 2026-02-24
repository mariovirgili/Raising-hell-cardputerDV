#include "ui_state_burial.h"

#include <Arduino.h>

#include "input.h"
#include "save_manager.h"
#include "sound.h"
#include "ui_runtime.h"
#include "ui_input_common.h"

void ui_showMessage(const char* msg);
void forceRenderUIOnce();

void uiBurialHandle(InputState& in)
{
  if (!(in.selectOnce || in.encoderPressOnce)) {
    uiDrainKb(in);
    clearInputLatch();
    return;
  }

  // Show toast and force at least one render before reboot
  ui_showMessage("Rest in peace...");
  forceRenderUIOnce();

  delay(950);

  soundResetDeathDirgeLatch();
  saveManagerDeletePetOnly();

  delay(50);
  ESP.restart();
}