#include "screens/Modules.h"
#include "ui/UI.h"

// Analog voltmeter on GPIO35 (ADC1_CH7, input-only, exposed on the CYD header).
namespace {
constexpr int ADC_PIN = 35;
constexpr int HN = 80;
const char* const kFoot[] = {"تحديث"};
}  // namespace

void AdcScreen::onEnter() {
  ui::clearContent(true);
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);
  hi_ = 0;
  for (int i = 0; i < HN; i++) hist_[i] = 0;
  last_ = 0;
  ui::footerBar(kFoot, nullptr, 1);
}

void AdcScreen::update(uint32_t now) {
  if (now - last_ < 100) return;
  last_ = now;
  int raw = 0;
  for (int i = 0; i < 8; i++) raw += analogRead(ADC_PIN);
  raw /= 8;
  hist_[hi_] = raw;
  hi_ = (hi_ + 1) % HN;
  const float v = raw / 4095.0f * 3.3f;

  tft.fillRect(0, ui::CONTENT_Y, ui::W, ui::CONTENT_HB, Theme::BG);
  char b[16];
  snprintf(b, sizeof(b), "%.2f", v);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(Theme::FG, Theme::BG);
  tft.drawString(b, ui::W / 2 - 24, ui::CONTENT_Y + 34, 7);
  tft.setTextColor(Theme::MID, Theme::BG);
  tft.drawString("V", ui::W / 2 + 54, ui::CONTENT_Y + 40, 4);

  snprintf(b, sizeof(b), "RAW %d / 4095", raw);
  txt::draw(ui::W / 2, ui::CONTENT_Y + 66, b, Theme::DIM, Theme::BG, txt::CENTER, txt::FONT_S);
  snprintf(b, sizeof(b), "GPIO%d", ADC_PIN);
  txt::draw(ui::W / 2, ui::CONTENT_Y + 84, b, Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);

  const int16_t by = ui::FOOTER_Y - 6, bx = 12, bw = (ui::W - 24) / HN, h = 40;
  for (int i = 0; i < HN; i++) {
    const int idx = (hi_ + i) % HN;
    const int16_t bh = (int16_t)(h * hist_[idx] / 4095);
    tft.fillRect(bx + i * bw, by - bh, bw - 1 > 0 ? bw - 1 : 1, bh, Theme::FG);
  }
  tft.drawFastHLine(bx, by, ui::W - 24, Theme::GRID);
  ui::footerBar(kFoot, nullptr, 1);
}

bool AdcScreen::onInput(const InputEvent& e) {
  if (e.tap() && ui::footerHit(e.x, e.y, 1) == 0) onEnter();
  return true;
}
