#include <BLEDevice.h>
#include "core/BleReceiver.h"
#include "core/Settings.h"
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "screens/Modules.h"
#include "ui/UI.h"

// Proximity radar: continuously scans and shows the nearest BLE device, with a
// blip whose distance from centre tracks its signal (closer = stronger).
namespace {
constexpr int16_t CX = 110, R = 76;
int16_t cy() { return ui::CONTENT_Y + 90; }
}  // namespace

void BleRadarScreen::onEnter() {
  ui::clearContent();
  bleRx::begin();
  bleRx::start(settings.bleSeconds);
  last_ = 0;
  draw("--", "", -120, 0);
}
void BleRadarScreen::onExit() { bleRx::end(); }

void BleRadarScreen::update(uint32_t now) {
  if (!bleRx::ready()) return;
  BLEScanResults r = bleRx::results();
  int best = -1, bestRssi = -200;
  for (int i = 0; i < r.getCount(); i++) {
    const int rs = r.getDevice(i).getRSSI();
    if (rs >= settings.minRssi && rs > bestRssi) {
      bestRssi = rs;
      best = i;
    }
  }
  char name[24] = "--", addr[18] = "";
  if (best >= 0) {
    BLEAdvertisedDevice d = r.getDevice(best);
    const std::string nm = d.getName();
    strncpy(name, nm.empty() ? "<بدون اسم>" : nm.c_str(), sizeof(name) - 1);
    strncpy(addr, d.getAddress().toString().c_str(), sizeof(addr) - 1);
  }
  const int cnt = r.getCount();
  bleRx::clear();
  bleRx::start(settings.bleSeconds);
  sweep_ += 55;
  if (sweep_ >= 360) sweep_ -= 360;
  last_ = now;
  draw(name, addr, bestRssi, cnt);
}

void BleRadarScreen::draw(const char* name, const char* addr, int rssi, int count) {
  const int16_t Y = cy();
  tft.fillRect(0, ui::CONTENT_Y, ui::W, ui::CONTENT_H, Theme::BG);
  // rings
  for (int i = 1; i <= 3; i++) tft.drawCircle(CX, Y, R * i / 3, Theme::GRID);
  tft.drawCircle(CX, Y, R, Theme::EDGE);
  tft.drawFastHLine(CX - R, Y, 2 * R, Theme::GRID);
  tft.drawFastVLine(CX, Y - R, 2 * R, Theme::GRID);
  // sweep line
  const float a = sweep_ * 0.0174533f;
  tft.drawLine(CX, Y, CX + cosf(a) * R, Y + sinf(a) * R, Theme::FG);
  // blip
  if (rssi > -120) {
    float dist = (rssi + 100) / 60.0f;  // -100..-40 -> 0..1 (near)
    dist = constrain(1.0f - dist, 0.05f, 1.0f);
    const int16_t br = (int16_t)(R * dist);
    const float ba = sweep_ * 0.0174533f;
    tft.fillCircle(CX + cosf(ba) * br, Y + sinf(ba) * br, 4, Theme::ALERT);
  }
  // info panel on the right
  const int16_t px = 196;
  txt::draw(ui::W - 10, ui::CONTENT_Y + 10, "اقرب جهاز", Theme::MID, Theme::BG, txt::RIGHT,
            txt::FONT_S);
  txt::draw(ui::W - 10, ui::CONTENT_Y + 30, name, Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_M);
  char b[32];
  snprintf(b, sizeof(b), "%d dBm", rssi > -120 ? rssi : 0);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(Theme::ACCENT, Theme::BG);
  tft.setTextPadding(90);
  tft.drawString(b, ui::W - 10, ui::CONTENT_Y + 58, 4);
  tft.setTextPadding(0);
  tft.setTextColor(Theme::DIM, Theme::BG);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(addr, ui::W - 10, ui::CONTENT_Y + 84, 1);
  snprintf(b, sizeof(b), "%d جهاز", count);
  txt::draw(ui::W - 10, ui::CONTENT_Y + 100, b, Theme::MID, Theme::BG, txt::RIGHT, txt::FONT_S);
  (void)px;
}

bool BleRadarScreen::onInput(const InputEvent& e) { return false; }
