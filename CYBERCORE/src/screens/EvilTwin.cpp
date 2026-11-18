#include <WiFi.h>
#include <esp_wifi.h>
#include "core/Settings.h"
#include <string.h>
#include "screens/Modules.h"
#include "ui/UI.h"

// Detects a possible "evil twin": the same SSID advertised by two or more
// different BSSIDs (MACs) - a classic sign someone cloned your network.
namespace {
constexpr int16_t LIST_Y = ui::CONTENT_Y + 34;
constexpr int16_t ROW_H = 26;
constexpr int VISIBLE = 4;
const char* const kFoot[] = {"تحديث", "اعلى", "اسفل", "حفظ SD"};
}  // namespace

void EvilTwinScreen::onEnter() {
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
void EvilTwinScreen::onExit() {
  esp_wifi_scan_stop();
  WiFi.scanDelete();
  WiFi.mode(WIFI_MODE_NULL);
}

void EvilTwinScreen::run() {
  n_ = wifiCollect(aps_, MAX);
  nflag_ = 0;
  for (int i = 0; i < n_ && nflag_ < 20; i++) {
    for (int j = 0; j < n_; j++) {
      if (j == i || aps_[i].hidden || aps_[j].hidden) continue;
      if (strcmp(aps_[i].ssid, aps_[j].ssid) == 0 && memcmp(aps_[i].bssid, aps_[j].bssid, 6) != 0) {
        flags_[nflag_++] = i;
        break;
      }
    }
  }
  top_ = 0;
  if (settings.autoSave && n_) saveWifiResults(aps_, n_, "twins");
}

void EvilTwinScreen::update(uint32_t) {
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

void EvilTwinScreen::draw() {
  ui::clearContent(true);
  if (nflag_ == 0) {
    icon::draw(tft, icon::SHIELD, ui::W / 2, ui::CONTENT_Y + 50, 26, Theme::FG);
    txt::draw(ui::W / 2, ui::CONTENT_Y + 84, "لم ترصد اسماء مكررة", Theme::FG, Theme::BG,
              txt::CENTER, txt::FONT_M);
    ui::footerBar(kFoot, nullptr, 4);
    return;
  }
  char s[40];
  snprintf(s, sizeof(s), "%d اسم مكرر - تحقق من المصدر", nflag_);
  txt::draw(ui::W - 12, ui::CONTENT_Y + 6, s, Theme::ALERT, Theme::BG, txt::RIGHT, txt::FONT_M);

  for (int k = top_; k < top_ + VISIBLE && k < nflag_; k++) {
    const ApInfo& a = aps_[flags_[k]];
    const int16_t y = LIST_Y + (k - top_) * ROW_H;
    tft.fillRect(0, y, ui::W, ROW_H - 1, Theme::BG);
    tft.drawFastHLine(8, y + ROW_H - 2, ui::W - 16, Theme::GRID);
    icon::draw(tft, icon::ALERT, 16, y + 11, 8, Theme::ALERT);
    char b[24];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X  C%d", a.bssid[0], a.bssid[1], a.bssid[2],
             a.bssid[3], a.bssid[4], a.bssid[5], a.channel);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(Theme::DIM, Theme::BG);
    tft.drawString(b, 30, y + 12, 1);
    txt::draw(ui::W - 10, y + 2, a.ssid, Theme::WARN, Theme::BG, txt::RIGHT, txt::FONT_M);
  }
  ui::footerBar(kFoot, nullptr, 4);
}

bool EvilTwinScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  const int f = ui::footerHit(e.x, e.y, 4);
  if (f == 0) {
    if (!busy_) onEnter();
    return true;
  }
  if (f == 3) {
    ui::toast(saveWifiResults(aps_, n_, "twins") ? "تم الحفظ" : "فشل الحفظ SD");
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
    if (top_ + VISIBLE < nflag_) {
      top_ += VISIBLE;
      draw();
    }
    return true;
  }
  return true;
}
