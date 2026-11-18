#include "core/Input.h"
#include "config.h"
#include "core/Touch.h"

void Input::begin() {
  if (PIN_BTN_BACK != PIN_NONE) pinMode(PIN_BTN_BACK, INPUT_PULLUP);
  for (uint8_t pin : {PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_OK})
    if (pin != PIN_NONE) pinMode(pin, INPUT_PULLUP);
  touch::begin();
}

InputEvent Input::poll() {
  const uint32_t now = millis();

  // ---- touch: detect a tap (short press, little movement) on release ----
  if (touch::available() && now - lastRead_ >= 16) {
    lastRead_ = now;
    const touch::Point p = touch::read();
    if (p.pressed) {
      releaseAt_ = 0;
      if (!down_) {
        down_ = true;
        sx_ = lx_ = p.x;
        sy_ = ly_ = p.y;
        downAt_ = now;
      } else {
        lx_ = p.x;
        ly_ = p.y;
      }
    } else if (down_) {
      if (!releaseAt_) releaseAt_ = now;
      if (now - releaseAt_ < 40) return {};
      down_ = false;
      const uint32_t dur = now - downAt_;
      if (dur >= 35 && dur < 1800 && abs(lx_ - sx_) < 32 && abs(ly_ - sy_) < 32)
        return InputEvent{Btn::Tap, Evt::Press, sx_, sy_};
    }
  }

  // ---- BOOT button -> back ----
  if (PIN_BTN_BACK != PIN_NONE) {
    const bool raw = digitalRead(PIN_BTN_BACK) == LOW;
    if (raw != bootRaw_) {
      bootRaw_ = raw;
      bootChange_ = now;
    }
    if (now - bootChange_ >= BTN_DEBOUNCE_MS && raw != bootStable_) {
      bootStable_ = raw;
      if (raw)
        bootDown_ = now;
      else {
        uint32_t held = now - bootDown_;
        return InputEvent{held >= 1800  ? Btn::Back
                          : held >= 550 ? Btn::Select
                                        : Btn::Next,
                          Evt::Press};
      }
    }
  }
  const uint8_t pins[] = {PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_OK};
  const Btn buttons[] = {Btn::Up, Btn::Down, Btn::Select};
  for (int i = 0; i < 3; ++i) {
    if (pins[i] == PIN_NONE) continue;
    bool held = digitalRead(pins[i]) == LOW;
    if (held != keys_[i] && now - keyAt_[i] >= BTN_DEBOUNCE_MS) {
      keys_[i] = held;
      keyAt_[i] = now;
      if (held) return InputEvent{buttons[i], Evt::Press};
    }
  }
  return InputEvent{};
}
