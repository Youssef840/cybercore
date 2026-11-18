#include <WiFi.h>
#include "config.h"
#include "screens/Modules.h"
#include "ui/UI.h"

namespace {
constexpr int16_t Y0 = ui::CONTENT_Y + 6;
constexpr int16_t ROW = 24;

void row(int i, const char* labelAr, const char* value, uint16_t vc) {
  const int16_t y = Y0 + i * ROW;
  tft.fillRect(0, y, ui::W, ROW - 2, Theme::BG);
  // label (right, Arabic)
  txt::draw(ui::W - 10, y + 3, labelAr, Theme::MID, Theme::BG, txt::RIGHT, txt::FONT_S);
  // value (left, latin)
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(vc, Theme::BG);
  tft.drawString(value, 12, y + 11, 2);
  tft.drawFastHLine(10, y + ROW - 3, ui::W - 20, Theme::GRID);
}
}  // namespace

void SysInfoScreen::onEnter() {
  ui::clearContent();
  char b[40];
  row(0, "الطراز", ESP.getChipModel(), Theme::FG);
  snprintf(b, sizeof(b), "%lu MHz", (unsigned long)ESP.getCpuFreqMHz());
  row(1, "التردد", b, Theme::FG);
  snprintf(b, sizeof(b), "%u MB", ESP.getFlashChipSize() / (1024 * 1024));
  row(2, "الفلاش", b, Theme::FG);
  row(4, "العنوان MAC", WiFi.macAddress().c_str(), Theme::ACCENT);
  row(6, "اصدار البرنامج", "CYBERCORE v" FW_VERSION, Theme::ACCENT);
  last_ = 0;
}

void SysInfoScreen::update(uint32_t now) {
  if (now - last_ < 1000) return;
  last_ = now;
  char b[32];
  snprintf(b, sizeof(b), "%u KB", ESP.getFreeHeap() / 1024);
  row(3, "الذاكرة الحرة", b, Theme::FG);
  const uint32_t s = now / 1000;
  snprintf(b, sizeof(b), "%02lu:%02lu:%02lu", (unsigned long)(s / 3600),
           (unsigned long)((s / 60) % 60), (unsigned long)(s % 60));
  row(5, "مدة التشغيل", b, Theme::FG);
}
