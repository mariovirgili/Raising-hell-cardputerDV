#include "graphics_assets.h"

#include <Arduino.h>
// ------------------------------------------------------------
// SHARED FRAMEBUFFER — used for SD-loaded backgrounds
// ------------------------------------------------------------
uint16_t frameBuffer[240 * 320];


// ------------------------------------------------------------
// BACKGROUND POINTERS — all simply alias frameBuffer
// (we load one image at a time into this buffer)
// ------------------------------------------------------------
uint16_t* hell_bg  = frameBuffer;
uint16_t* inv_bg   = frameBuffer;
uint16_t* sleep_bg = frameBuffer;
uint16_t* set_bg   = frameBuffer;
uint16_t* shop_bg  = frameBuffer;
uint16_t* power_bg = frameBuffer;


// ------------------------------------------------------------
// ICONS IN PROGMEM — temporary blank placeholders
// Replace with real icons later.
// ------------------------------------------------------------

#define BLANK_ICON_32 {0}
#define BLANK_ICON_48 {0}

// Feed icon
const uint16_t icon_feed[32 * 32] PROGMEM = BLANK_ICON_32;

// Play icon
const uint16_t icon_play[32 * 32] PROGMEM = BLANK_ICON_32;

// Sleep icon
const uint16_t icon_sleep[32 * 32] PROGMEM = BLANK_ICON_32;

// Inventory icon
const uint16_t icon_inv[32 * 32] PROGMEM = BLANK_ICON_32;

// Shop icon
const uint16_t icon_shop[32 * 32] PROGMEM = BLANK_ICON_32;

// Hell coin icon
const uint16_t hell_coin[32 * 32] PROGMEM = BLANK_ICON_32;


// ------------------------------------------------------------
// HEART ICONS (OUTLINE + FULL)
// ------------------------------------------------------------
const uint16_t heart_out[48 * 48] PROGMEM = BLANK_ICON_48;
const uint16_t heart_full[48 * 48] PROGMEM = BLANK_ICON_48;
