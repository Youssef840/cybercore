#include <WiFi.h>
#include <esp_wifi.h>
#include "core/Settings.h"
#include "screens/Modules.h"
#include "ui/UI.h"

namespace {
constexpr int16_t LIST_Y = ui::CONTENT_Y + 2;
constexpr int16_t ROW_H = 30;
const char* const kFoot[] = {"تحديث", "اعلى", "اسفل", "حفظ SD"};

void bars(int16_t x, int16_t y, int rssi, uint16_t c) {
  int lit = (rssi + 95) / 14;
  lit = constrain(lit, 0, 4);
  for (int i = 0; i < 4; i++) {
    const int16_t bh = 4 + i * 4, bx = x + i * 5, by = y + 16 - bh;
    if (i < lit)
      tft.fillRect(bx, by, 3, bh, c);
    else
      tft.drawRect(bx, by, 3, bh, Theme::GRID);
  }
}
}  // namespace

void WifiScanScreen::onEnter() { startScan(); }
void WifiScanScreen::onExit() {
  esp_wifi_scan_stop();
  WiFi.scanDelete();
  WiFi.mode(WIFI_MODE_NULL);
}

void WifiScanScreen::footer() { ui::footerBar(kFoot, nullptr, 4); }

void WifiScanScreen::startScan() {
  ui::clearContent(true);
  ui::toast("جاري البحث...", Theme::WARN);
  footer();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.scanDelete();
  int result = WiFi.scanNetworks(true, true, true, settings.scanMs, settings.channel);
  scanning_ = result == WIFI_SCAN_RUNNING;
  if (!scanning_) {
    ui::toast("فشل بدء المسح", Theme::WARN);
    scannedAt_ = millis();
  }
}

void WifiScanScreen::update(uint32_t) {
  if (!scanning_) {
    if (settings.autoRefresh && millis() - scannedAt_ >= settings.refreshSeconds * 1000UL)
      startScan();
    return;
  }
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
  n_ = wifiCollect(aps_, MAX);
  scannedAt_ = millis();
  if (settings.autoSave && n_) saveWifiResults(aps_, n_, "wifi");
  scanning_ = false;
  top_ = 0;
  Serial.printf("[wifi] scan complete count=%d\n", n_);
  drawList();
}

void WifiScanScreen::drawList() {
  ui::clearContent(true);
  if (n_ == 0) {
    ui::toast("لا توجد شبكات", Theme::DIM);
  } else {
    for (int i = top_; i < top_ + VISIBLE && i < n_; i++) drawRow(i);
  }
  footer();
}

void WifiScanScreen::drawRow(int i) {
  const int slot = i - top_;
  const int16_t y = LIST_Y + slot * ROW_H;
  const ApInfo& a = aps_[i];
  const uint16_t sc = a.rssi > -67 ? Theme::FG : (a.rssi > -80 ? Theme::WARN : Theme::ALERT);
  tft.fillRect(0, y, ui::W, ROW_H - 1, Theme::BG);
  tft.drawFastHLine(8, y + ROW_H - 2, ui::W - 16, Theme::GRID);

  bars(10, y + 6, a.rssi, sc);
  char b[12];
  snprintf(b, sizeof(b), "%d", a.rssi);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(sc, Theme::BG);
  tft.drawString(b, 34, y + 14, 1);
  snprintf(b, sizeof(b), "C%d", a.channel);
  tft.setTextColor(Theme::DIM, Theme::BG);
  tft.drawString(b, 64, y + 14, 1);

  const bool open = a.enc == WIFI_AUTH_OPEN;
  icon::draw(tft, icon::LOCK, 94, y + 13, 5, open ? Theme::ALERT : Theme::MID);
  tft.setTextColor(open ? Theme::ALERT : Theme::MID, Theme::BG);
  tft.drawString(encNameAr(a.enc), 106, y + 14, 1);

  txt::draw(ui::W - 10, y + 3, a.ssid, Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_M);
}

bool WifiScanScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  const int f = ui::footerHit(e.x, e.y, 4);
  if (f == 0) {
    if (!scanning_) startScan();
    return true;
  }
  if (f == 3) {
    ui::toast(saveWifiResults(aps_, n_, "wifi") ? "تم الحفظ" : "فشل الحفظ SD");
    return true;
  }
  if (f == 1) {
    if (top_ > 0) {
      top_ = max(0, top_ - VISIBLE);
      drawList();
    }
    return true;
  }
  if (f == 2) {
    if (top_ + VISIBLE < n_) {
      top_ += VISIBLE;
      drawList();
    }
    return true;
  }
  return true;
}
