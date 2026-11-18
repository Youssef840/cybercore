#pragma once
// ============================================================================
//  Touch - XPT2046 resistive touch controller, bit-banged on its own pins.
//  Bit-banging avoids sharing VSPI with the CC1101 driver (which keeps calling
//  SPI.begin/SPI.end) and HSPI with the display.
// ============================================================================
#include <Arduino.h>

namespace touch {
struct Point {
  int16_t x = -1, y = -1;     // screen coordinates
  uint16_t rawX = 0, rawY = 0, z = 0;
  bool pressed = false;
};

bool available();             // false on boards without a touch panel
void begin();
Point read();
extern bool debug;            // print raw readings to Serial while touched
}  // namespace touch
