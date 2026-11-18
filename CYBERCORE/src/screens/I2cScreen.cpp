#include <Wire.h>
#include "screens/Modules.h"
#include "ui/UI.h"

// Scans the I2C bus for connected sensors/modules (maker tool).
// CYD exposes GPIO22 (SDA) and GPIO27 (SCL) on the extension header.
namespace {
constexpr int SDA_PIN = 22, SCL_PIN = 27;
const char* const kFoot[] = {"تحديث"};
}  // namespace

void I2cScreen::run() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  n_ = 0;
  for (uint8_t a = 1; a < 127 && n_ < 16; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) found_[n_++] = a;
    delay(2);
  }
}

void I2cScreen::onEnter() {
  ui::clearContent(true);
  ui::footerBar(kFoot, nullptr, 1);
  run();

  char s[48];
  snprintf(s, sizeof(s), "SDA=%d  SCL=%d", SDA_PIN, SCL_PIN);
  txt::draw(ui::W - 12, ui::CONTENT_Y + 6, s, Theme::MID, Theme::BG, txt::RIGHT, txt::FONT_S);

  if (n_ == 0) {
    ui::toast("لا يوجد جهاز موصول", Theme::DIM);
    return;
  }
  snprintf(s, sizeof(s), "عدد الاجهزة: %d", n_);
  txt::draw(ui::W - 12, ui::CONTENT_Y + 28, s, Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_M);
  for (int i = 0; i < n_; i++) {
    const int16_t x = 24 + (i % 4) * 72, y = ui::CONTENT_Y + 56 + (i / 4) * 23;
    ui::tile(x, y, 62, 21, false);
    char h[8];
    snprintf(h, sizeof(h), "0x%02X", found_[i]);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::ACCENT, Theme::TILE);
    tft.drawString(h, x + 31, y + 10, 2);
  }
}

bool I2cScreen::onInput(const InputEvent& e) {
  if (e.tap() && ui::footerHit(e.x, e.y, 1) == 0) onEnter();
  return true;
}
