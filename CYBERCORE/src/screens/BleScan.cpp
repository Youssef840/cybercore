#include <BLEDevice.h>
#include "core/BleReceiver.h"
#include "core/Settings.h"
#include "core/Storage.h"
#include "core/App.h"
#include "screens/Extras.h"
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "screens/Modules.h"
#include "ui/UI.h"

namespace {
constexpr int16_t LIST_Y = ui::CONTENT_Y + 2;
constexpr int16_t ROW_H = 30;
const char* const kFoot[] = {"تحديث", "اعلى", "اسفل", "حفظ SD"};

void bars(int16_t x, int16_t y, int rssi, uint16_t c) {
  int lit = (rssi + 100) / 13;
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

void BleScanScreen::onEnter() {
  ui::clearContent(true);
  ui::toast("جاري البحث...", Theme::WARN);
  ui::footerBar(kFoot, nullptr, 4);
  bleRx::begin();
  scanning_ = bleRx::start(settings.bleSeconds);
  scannedAt_ = millis();
}

void BleScanScreen::onExit() { bleRx::end(); }

void BleScanScreen::update(uint32_t) {
  if (!scanning_) {
    if (settings.autoRefresh && millis() - scannedAt_ >= settings.refreshSeconds * 1000UL) {
      scanning_ = bleRx::start(settings.bleSeconds);
      scannedAt_ = millis();
    }
    return;
  }
  if (!bleRx::ready()) return;
  BLEScanResults r = bleRx::results();
  n_ = 0;
  for (int j = 0; j < r.getCount() && n_ < MAX; j++) {
    BLEAdvertisedDevice d = r.getDevice(j);
    if (d.getRSSI() < settings.minRssi) continue;
    int i = n_++;
    const std::string nm = d.getName();
    strncpy(dev_[i].name, nm.empty() ? "<بدون اسم>" : nm.c_str(), sizeof(dev_[i].name) - 1);
    dev_[i].name[sizeof(dev_[i].name) - 1] = 0;
    strncpy(dev_[i].addr, d.getAddress().toString().c_str(), sizeof(dev_[i].addr) - 1);
    dev_[i].addr[sizeof(dev_[i].addr) - 1] = 0;
    dev_[i].rssi = d.getRSSI();
  }
  bleRx::clear();
  scannedAt_ = millis();
  if (settings.autoSave) save();
  scanning_ = false;
  top_ = 0;
  Serial.printf("[ble] scan complete count=%d\n", n_);
  draw();
}

void BleScanScreen::draw() {
  ui::clearContent(true);
  if (n_ == 0)
    ui::toast("لا توجد اجهزة", Theme::DIM);
  else
    for (int i = top_; i < top_ + VISIBLE && i < n_; i++) drawRow(i);
  ui::footerBar(kFoot, nullptr, 4);
}

void BleScanScreen::drawRow(int i) {
  const int slot = i - top_;
  const int16_t y = LIST_Y + slot * ROW_H;
  const Dev& d = dev_[i];
  ui::addFocus(0, y, ui::W, ROW_H - 1);
  const uint16_t sc = d.rssi > -70 ? Theme::FG : (d.rssi > -85 ? Theme::WARN : Theme::ALERT);
  tft.fillRect(0, y, ui::W, ROW_H - 1, Theme::BG);
  tft.drawFastHLine(8, y + ROW_H - 2, ui::W - 16, Theme::GRID);
  bars(10, y + 6, d.rssi, sc);
  char b[12];
  snprintf(b, sizeof(b), "%d", d.rssi);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(sc, Theme::BG);
  tft.drawString(b, 34, y + 8, 1);
  tft.setTextColor(Theme::DIM, Theme::BG);
  tft.drawString(d.addr, 34, y + 20, 1);
  txt::draw(ui::W - 10, y + 3, d.name, Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_M);
}

bool BleScanScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  const int f = ui::footerHit(e.x, e.y, 4);
  if (f == 0) {
    if (!scanning_) {
      scanning_ = bleRx::start(settings.bleSeconds);
      scannedAt_ = millis();
    }
    return true;
  }
  if (f == 3) {
    ui::toast(save() ? "تم الحفظ" : "فشل الحفظ SD");
    return true;
  }
  if (e.y >= LIST_Y && e.y < LIST_Y + VISIBLE * ROW_H) {
    int i = top_ + (e.y - LIST_Y) / ROW_H;
    if (i < n_) {
      trackBle(dev_[i].addr, dev_[i].name);
      App::instance().open(bleTracker());
    }
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

bool BleScanScreen::save() {
  if (n_ <= 0) return false;
  for (int i = 0; i < n_; ++i) {
    auto& d = dev_[i];
    String line =
        String(millis()) + "," + storage::quote(d.name) + "," + d.addr + "," + String(d.rssi);
    if (!storage::append("ble", line, "uptime_ms,name,address,rssi")) return false;
  }
  return true;
}
