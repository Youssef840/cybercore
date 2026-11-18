#include <WiFi.h>
#include <esp_wifi.h>
#include "core/Settings.h"
#include "screens/Modules.h"
#include "ui/UI.h"

namespace {
constexpr int16_t PANEL_H = 50;
constexpr int16_t LIST_Y = ui::CONTENT_Y + PANEL_H + 4;
constexpr int16_t ROW_H = 28;
const char* const kFoot[] = {"تحديث", "اعلى", "اسفل", "حفظ SD"};

const char* verdictAr(int r) { return r == 0 ? "ممتاز" : r == 1 ? "جيد" : r == 2 ? "ضعيف" : "خطر"; }
uint16_t verdictColor(int r) {
  return r == 0 ? Theme::ACCENT : r == 1 ? Theme::FG : r == 2 ? Theme::WARN : Theme::ALERT;
}
}  // namespace

int WifiAuditScreen::riskOf(uint8_t enc) {
  switch (enc) {
    case WIFI_AUTH_WPA3_PSK:
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return 0;
    case WIFI_AUTH_WPA2_PSK:
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return 1;
    case WIFI_AUTH_WPA_PSK:
    case WIFI_AUTH_WPA_WPA2_PSK:
      return 2;
    default:
      return 3;
  }
}

void WifiAuditScreen::onEnter() {
  ui::clearContent(true);
  ui::toast("جاري الفحص...", Theme::WARN);
  ui::footerBar(kFoot, nullptr, 4);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.scanDelete();
  busy_ =
      WiFi.scanNetworks(true, true, true, settings.scanMs, settings.channel) == WIFI_SCAN_RUNNING;
  scannedAt_ = millis();
}
void WifiAuditScreen::onExit() {
  esp_wifi_scan_stop();
  WiFi.scanDelete();
  WiFi.mode(WIFI_MODE_NULL);
}
void WifiAuditScreen::footer() { ui::footerBar(kFoot, nullptr, 4); }

void WifiAuditScreen::update(uint32_t) {
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

void WifiAuditScreen::run() {
  n_ = wifiCollect(aps_, MAX);
  secure_ = weak_ = danger_ = 0;
  for (int i = 0; i < n_; i++) {
    const int r = riskOf(aps_[i].enc);
    if (r <= 1)
      secure_++;
    else if (r == 2)
      weak_++;
    else
      danger_++;
  }
  top_ = 0;
  if (settings.autoSave && n_) saveWifiResults(aps_, n_, "audit");
}

void WifiAuditScreen::draw() {
  ui::clearContent(true);
  // summary panel
  ui::tile(6, ui::CONTENT_Y + 2, ui::W - 12, PANEL_H, false);
  char line[64];
  snprintf(line, sizeof(line), "امن %d    ضعيف %d    خطر %d", secure_, weak_, danger_);
  txt::draw(ui::W - 14, ui::CONTENT_Y + 7, line, Theme::FG, Theme::TILE, txt::RIGHT, txt::FONT_M);
  txt::draw(ui::W - 14, ui::CONTENT_Y + 30, "تقدير حسب التشفير فقط", Theme::MID, Theme::TILE,
            txt::RIGHT, txt::FONT_S);

  if (n_ == 0)
    ui::toast("لا توجد شبكات", Theme::DIM);
  else
    for (int i = top_; i < top_ + VISIBLE && i < n_; i++) drawRow(i);
  footer();
}

void WifiAuditScreen::drawRow(int i) {
  const int slot = i - top_;
  const int16_t y = LIST_Y + slot * ROW_H;
  const ApInfo& a = aps_[i];
  const int risk = riskOf(a.enc);
  tft.fillRect(0, y, ui::W, ROW_H - 1, Theme::BG);
  tft.drawFastHLine(8, y + ROW_H - 2, ui::W - 16, Theme::GRID);

  // verdict badge (left)
  const uint16_t vc = verdictColor(risk);
  tft.drawRoundRect(8, y + 3, 52, ROW_H - 9, 3, vc);
  txt::draw(34 + txt::width(verdictAr(risk), txt::FONT_S) / 2, y + 4, verdictAr(risk), vc,
            Theme::BG, txt::RIGHT, txt::FONT_S);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(Theme::GRID, Theme::BG);
  tft.drawString(encNameAr(a.enc), 66, y + 12, 1);
  txt::draw(ui::W - 10, y + 2, a.ssid, Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_M);
}

bool WifiAuditScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  const int f = ui::footerHit(e.x, e.y, 4);
  if (f == 0) {
    if (!busy_) onEnter();
    return true;
  }
  if (f == 3) {
    ui::toast(saveWifiResults(aps_, n_, "audit") ? "تم الحفظ" : "فشل الحفظ SD");
    return true;
  }
  if (f == 1) {
    if (top_ > 0) {
      top_ = max(0, top_ - VISIBLE);
      draw();
    }
    return true;
  }
  if (f == 2) {
    if (top_ + VISIBLE < n_) {
      top_ += VISIBLE;
      draw();
    }
    return true;
  }
  return true;
}
