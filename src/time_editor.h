#pragma once

#include <stdbool.h>

// ----------------------------------------------------------------------------
// Simple time editor helpers used by the SET_TIME flow.
//
// The project already has `time_state.{h,cpp}` holding the live clock fields.
// This editor provides a tiny state machine over those fields (hour/minute)
// and commit behavior.
// ----------------------------------------------------------------------------

void timeEditorBegin(bool force24h);
void timeEditorPrevField();
void timeEditorNextField();
void timeEditorInc();
void timeEditorDec();

void timeStateCommitFromEditor();

int  timeEditorHour();
int  timeEditorMinute();
int  timeEditorField(); // 0=hour, 1=minute
