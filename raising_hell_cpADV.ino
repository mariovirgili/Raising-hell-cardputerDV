/*
  Raising Hell — Cardputer ADV Edition
  -----------------------------------
  Virtual pet game for M5Stack Cardputer ADV (ESP32).

  Build (Arduino IDE):
    - Arduino IDE: 1.8.19
    - Board: M5Cardputer (esp32 core via M5Stack package)
    - Flash: 4MB / QIO / 240MHz
    - Partition: Huge APP (3MB No OTA / 1MB SPIFFS)
    - Upload: 921600

  Assets:
    - Runs with SD card assets under:
        /raising_hell/...
      See README.md for the expected folder structure and required files.

  Code layout:
    - app_loop.cpp / app_setup.cpp: main loop + boot pipeline
    - flow_*.cpp: UI/boot flows and transitions (no rendering here)
    - *_state.*: small state holders (runtime flags, settings nav, brightness, etc.)
    - graphics.cpp: rendering, background cache, UI drawing
    - save_manager.cpp: saves + persistence

  Project rules (refactor notes):
    - New code should not include globals.h.
    - Prefer *_state modules for shared state.
    - UI transitions should use requestFullUIRedraw() + clearInputLatch().
    - Keep debug toggles in debug.h only.

  More docs:
    - README.md (setup + SD assets)
    - docs/ (architecture + troubleshooting)
*/
#include <Arduino.h>
#include "app_setup.h"
#include "app_loop.h"

void setup() {
  appSetup();
}

void loop() {
  appMainLoopTick();
}
