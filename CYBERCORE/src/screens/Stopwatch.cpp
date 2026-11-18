#include "screens/Modules.h"
#include "ui/UI.h"

namespace {
const char* const kFootRun[] = {"وقف", "تصفير"};
const char* const kFootStop[] = {"ابدأ", "تصفير"};
}  // namespace

void StopwatchScreen::onEnter() {
  running_ = false;
  acc_ = 0;
  draw();
}

void StopwatchScreen::draw() {
  tft.fillRect(0, ui::CONTENT_Y, ui::W, ui::CONTENT_HB, Theme::BG);
  const uint32_t ms = acc_ + (running_ ? millis() - start_ : 0);
  const uint32_t cs = (ms / 10) % 100, s = (ms / 1000) % 60, m = ms / 60000;
  char b[16];
  snprintf(b, sizeof(b), "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(running_ ? Theme::FG : Theme::MID, Theme::BG);
  tft.drawString(b, ui::W / 2 - 24, ui::CONTENT_Y + 54, 7);
  snprintf(b, sizeof(b), ".%02lu", (unsigned long)cs);
  tft.setTextColor(Theme::ACCENT, Theme::BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(b, ui::W / 2 + 56, ui::CONTENT_Y + 66, 6);
  ui::footerBar(running_ ? kFootRun : kFootStop, nullptr, 2);
}

void StopwatchScreen::update(uint32_t now) {
  if (running_ && now - lastDraw_ >= 40) { lastDraw_ = now; draw(); }
}

bool StopwatchScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  const int f = ui::footerHit(e.x, e.y, 2);
  if (f == 0) {
    if (running_) { acc_ += millis() - start_; running_ = false; }
    else { start_ = millis(); running_ = true; }
    draw();
  } else if (f == 1) {
    running_ = false;
    acc_ = 0;
    draw();
  }
  return true;
}
