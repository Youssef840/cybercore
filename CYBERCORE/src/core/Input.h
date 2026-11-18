#pragma once
// ============================================================================
//  Input - direct touch (tap with coordinates) + the BOOT button as "back".
//  Screens hit-test tap coordinates (tiles, rows, on-screen buttons).
// ============================================================================
#include <Arduino.h>

enum class Btn : uint8_t { None, Tap, Back, Up, Down, Select, Next, Prev };
enum class Evt : uint8_t { None, Press };

struct InputEvent {
  Btn btn = Btn::None;
  Evt type = Evt::None;
  int16_t x = -1, y = -1;  // screen coords for a Tap

  InputEvent() = default;
  InputEvent(Btn b, Evt t, int16_t px = -1, int16_t py = -1) : btn(b), type(t), x(px), y(py) {}

  bool is(Btn b, Evt t) const { return btn == b && type == t; }
  bool tap() const { return btn == Btn::Tap; }
};

class Input {
 public:
  void begin();
  InputEvent poll();

 private:
  // touch gesture state
  bool down_ = false;
  int16_t sx_ = 0, sy_ = 0, lx_ = 0, ly_ = 0;
  uint32_t downAt_ = 0, lastRead_ = 0, releaseAt_ = 0;
  // BOOT button
  bool bootStable_ = false, bootRaw_ = false;
  uint32_t bootChange_ = 0, bootDown_ = 0;
  bool keys_[3] = {};
  uint32_t keyAt_[3] = {};
};
