#ifndef FEED_H
#define FEED_H

#include "input.h"

void drawFeedMenu();
void feedPet(int mode);              // 0 snack, 1 until full
void handleFeedInput(const InputState& in);

#endif
