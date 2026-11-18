#include <WiFi.h>
#include <esp_wifi.h>
#include "core/Settings.h"
#include "screens/Modules.h"
#include "ui/UI.h"

namespace {
constexpr int CH_MAX = 13;
constexpr int16_t GX = 24, GW = ui::W - 36;
constexpr int16_t GTOP = ui::CONTENT_Y + 40, GBOT = ui::FOOTER_Y - 18;
const char* const kFoot[] = {"تحديث"};
}  // namespace

void ChannelScreen::onEnter() {
  ui::clearContent(true);
  ui::toast("جاري التحليل...", Theme::WARN);
  ui::footerBar(kFoot, nullptr, 1);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.scanDelete();
  busy_ =
      WiFi.scanNetworks(true, true, true, settings.scanMs, settings.channel) == WIFI_SCAN_RUNNING;
  scannedAt_ = millis();
}
void ChannelScreen::onExit() {
  esp_wifi_scan_stop();
  WiFi.scanDelete();
  WiFi.mode(WIFI_MODE_NULL);
}

void ChannelScreen::run() {
  static ApInfo aps[32];
  n_ = wifiCollect(aps, 32);
  for (int i = 0; i <= 14; i++) {
    count_[i] = 0;
    best_[i] = -120;
  }
  for (int i = 0; i < n_; i++) {
    const int c = aps[i].channel;
    if (c >= 1 && c <= 14) {
      count_[c]++;
      if (aps[i].rssi > best_[c]) best_[c] = aps[i].rssi;
    }
  }
  busiest_ = 1;
  for (int c = 2; c <= CH_MAX; c++)
    if (count_[c] > count_[busiest_]) busiest_ = c;
}

void ChannelScreen::update(uint32_t) {
  if (!busy_) {
    if (settings.autoRefresh && millis() - scannedAt_ >= settings.refreshSeconds * 1000UL)
      onEnter();
    return;
  }
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
  run();
  busy_ = false;
  scannedAt_ = millis();
  draw();
}

void ChannelScreen::draw() {
  ui::clearContent(true);
  char s[48];
  snprintf(s, sizeof(s), "%d شبكة  -  الازحم: قناة %d", n_, busiest_);
  txt::draw(ui::W - 12, ui::CONTENT_Y + 6, s, Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_S);

  int maxc = 1;
  for (int c = 1; c <= CH_MAX; c++)
    if (count_[c] > maxc) maxc = count_[c];
  const int16_t bw = GW / CH_MAX;
  const int16_t h = GBOT - GTOP;
  tft.drawFastHLine(GX - 2, GBOT, GW + 4, Theme::EDGE);

  for (int c = 1; c <= CH_MAX; c++) {
    const int16_t x = GX + (c - 1) * bw;
    const int16_t bh = count_[c] ? (int16_t)(h * count_[c] / maxc) : 0;
    const uint16_t col = c == busiest_    ? Theme::WARN
                         : count_[c] >= 3 ? Theme::ALERT
                         : count_[c]      ? Theme::FG
                                          : Theme::GRID;
    if (bh) {
      tft.fillRect(x + 2, GBOT - bh, bw - 4, bh, col);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(col, Theme::BG);
      tft.drawNumber(count_[c], x + bw / 2, GBOT - bh - 7, 1);
    } else {
      tft.drawFastHLine(x + 2, GBOT - 1, bw - 4, Theme::GRID);
    }
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(c == busiest_ ? Theme::WARN : Theme::DIM, Theme::BG);
    tft.drawNumber(c, x + bw / 2, GBOT + 8, 1);
  }
  ui::footerBar(kFoot, nullptr, 1);
}

bool ChannelScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  if (ui::footerHit(e.x, e.y, 1) == 0) onEnter();
  return true;
}
