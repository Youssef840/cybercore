#pragma once
// ============================================================================
//  Keyboard - on-screen tap keyboard (letters / caps / symbols). Modal helper
//  a screen drives: open(), draw(), tap() -> Result, text().
// ============================================================================
#include <Arduino.h>

namespace kb {
enum Result : uint8_t { NONE, TYPING, DONE, CANCEL };

void open(const char* title, bool mask = false, const char* initial = "");
void draw();
Result tap(int16_t x, int16_t y);   // feed a tap; returns state
const char* text();
}  // namespace kb
