#include "ui_state_name_pet.h"

#include <ctype.h>
#include <string.h>

#include "app_state.h"
#include "input.h"
#include "name_entry_state.h"
#include "new_pet_flow_state.h"
#include "pet.h"
#include "sound.h"
#include "ui_new_pet_flow.h"
#include "ui_runtime.h"
#include "ui_input_common.h"

void uiNamePetHandle(InputState& in)
{
  // One-time cleanup after entering the screen
  if (g_namePetJustOpened) {
    g_namePetJustOpened = false;
    uiDrainKb(in);
    inputForceClear();
    clearInputLatch();
  }

  bool changed = false;

  // Back cancels back to choose pet
  if (in.menuOnce) {
    // ESC is intentionally ignored on NAME_PET (it's used elsewhere in the boot flow).
    inputSetTextCapture(false);
    g_textCaptureMode = false;

    g_app.uiState = UIState::CHOOSE_PET;
    g_choosePetBlockHatchUntilRelease = true;

    requestUIRedraw();
    uiDrainKb(in);
    inputForceClear();
    clearInputLatch();
    return;
  }

  // Handle keyboard queue correctly (KeyEvent based)
  while (in.kbHasEvent()) {
    KeyEvent ev = in.kbPop();
    const uint8_t c = ev.code;

// Enter → finalize
if (c == '\n' || c == RH_KEY_ENTER) {
  if (g_pendingPetName[0] == '\0') {
    playBeep();
    continue;
  }
  finalizeNewPetFromName(in);
  return;
}

// Backspace
if (c == '\b' || c == RH_KEY_BACKSPACE) {
  size_t n = strnlen(g_pendingPetName, PET_NAME_MAX);
  if (n > 0) {
    g_pendingPetName[n - 1] = '\0';
    changed = true;
  }
  continue;
}

    // Printable ASCII only
    if (c < 32 || c > 126) continue;

    size_t n = strnlen(g_pendingPetName, PET_NAME_MAX);
    if (n >= PET_NAME_MAX) {
      playBeep();
      continue;
    }

    g_pendingPetName[n] = (char)c;
    g_pendingPetName[n + 1] = '\0';
    changed = true;
  }

  if (changed) requestUIRedraw();
  clearInputLatch();
}