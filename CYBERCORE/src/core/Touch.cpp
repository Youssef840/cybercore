#include "core/Touch.h"
#include "config.h"
#include "ui/UI.h"
#include "core/Settings.h"

namespace touch {
bool debug = false;

#if defined(HAS_TOUCH)
namespace {
constexpr uint16_t Z_THRESHOLD = 350;
uint32_t lastDebug = 0;

// SPI mode 0, MSB first: 8-bit command, then 16 clocks of result (12 bits)
uint16_t xfer(uint8_t cmd) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_TOUCH_MOSI, (cmd >> i) & 1);
    delayMicroseconds(1);
    digitalWrite(PIN_TOUCH_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_TOUCH_CLK, LOW);
  }
  digitalWrite(PIN_TOUCH_MOSI, LOW);
  uint16_t v = 0;
  for (int i = 0; i < 16; i++) {
    digitalWrite(PIN_TOUCH_CLK, HIGH);
    delayMicroseconds(1);
    v = (v << 1) | (digitalRead(PIN_TOUCH_MISO) ? 1 : 0);
    digitalWrite(PIN_TOUCH_CLK, LOW);
    delayMicroseconds(1);
  }
  return (v >> 3) & 0x0FFF;
}

uint16_t median3(uint16_t a, uint16_t b, uint16_t c) {
  if (a > b) std::swap(a, b);
  if (b > c) std::swap(b, c);
  if (a > b) std::swap(a, b);
  return b;
}
}  // namespace

bool available() { return true; }

void begin() {
  pinMode(PIN_TOUCH_CLK, OUTPUT);
  pinMode(PIN_TOUCH_MOSI, OUTPUT);
  pinMode(PIN_TOUCH_CS, OUTPUT);
  pinMode(PIN_TOUCH_MISO, INPUT);
  pinMode(PIN_TOUCH_IRQ, INPUT);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  digitalWrite(PIN_TOUCH_CLK, LOW);
}

Point read() {
  Point p;
  // PENIRQ goes LOW while the panel is pressed: skip the SPI work otherwise
  if (digitalRead(PIN_TOUCH_IRQ)) return p;

  digitalWrite(PIN_TOUCH_CS, LOW);
  const uint16_t z1 = xfer(0xB1);
  const uint16_t z2 = xfer(0xC1);
  p.z = z1 + 4095 - z2;
  if (p.z >= Z_THRESHOLD) {
    xfer(0x91);  // first conversion after switching is noisy - discard
    uint16_t xs[3], ys[3];
    for (int i = 0; i < 3; i++) {
      xs[i] = xfer(0xD1);
      ys[i] = xfer(0x91);
    }
    p.rawX = median3(xs[0], xs[1], xs[2]);
    p.rawY = median3(ys[0], ys[1], ys[2]);
  }
  xfer(0xD0);  // power down, re-enable PENIRQ
  digitalWrite(PIN_TOUCH_CS, HIGH);

  if (p.z < Z_THRESHOLD) return p;

  int x, y;
  if (settings.touchCal) {
    uint16_t rx = settings.touchSwap ? p.rawY : p.rawX, ry = settings.touchSwap ? p.rawX : p.rawY;
    x = map(rx, settings.touchX0, settings.touchX1, 0, ui::W - 1);
    y = map(ry, settings.touchY0, settings.touchY1, 0, ui::PHYSICAL_H - 1);
  } else {
    x = map(p.rawX, TOUCH_X_MIN, TOUCH_X_MAX, 0, ui::W - 1);
    y = map(p.rawY, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, ui::PHYSICAL_H - 1);
    if (TOUCH_FLIP_X) x = ui::W - 1 - x;
    if (TOUCH_FLIP_Y) y = ui::PHYSICAL_H - 1 - y;
  }
  p.x = constrain(x, 0, ui::W - 1);
  p.y = constrain(y, 0, ui::PHYSICAL_H - 1);
  p.pressed = true;

  if (debug && millis() - lastDebug > 120) {
    lastDebug = millis();
    Serial.printf("[touch] raw x=%u y=%u z=%u -> screen x=%d y=%d\n", p.rawX, p.rawY, p.z, p.x,
                  p.y);
  }
  return p;
}
#else
bool available() { return false; }
void begin() {}
Point read() { return Point(); }
#endif
}  // namespace touch
