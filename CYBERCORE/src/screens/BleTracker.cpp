#include "core/BleReceiver.h"
#include "core/Settings.h"
#include "core/Storage.h"
#include "core/App.h"
#include "screens/Modules.h"
#include "screens/Extras.h"
#include "ui/UI.h"
namespace {
char target[18] = "", label[24] = "";
class Tracker : public Screen {
  uint32_t lastSeen = 0, next = 0;
  int rssi = -127, low = -127, high = -127;
  bool busy = false;
  uint32_t detections = 0;
  String status;
  void draw() {
    ui::clearContent(true);
    txt::draw(160, 38, target[0] ? label : "اختر جهازا من الماسح", Theme::FG, Theme::BG,
              txt::CENTER, txt::FONT_M);
    txt::draw(160, 66, target, Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
    char b[80];
    snprintf(b, sizeof(b), "RSSI %d  min %d  max %d", rssi, low, high);
    txt::draw(160, 96, b, Theme::ACCENT, Theme::BG, txt::CENTER, txt::FONT_S);
    snprintf(b, sizeof(b), "seen %lu   age %lu s", (unsigned long)detections,
             (unsigned long)(lastSeen ? (millis() - lastSeen) / 1000 : 0));
    txt::draw(160, 122, b, Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
    txt::draw(160, 150, status.c_str(), Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
    const char* f[] = {"اختيار جهاز", "مسح", "حفظ SD"};
    ui::footerBar(f, nullptr, 3);
  }
  bool save() {
    return storage::append(
        "ble_track",
        String(millis()) + "," + target + "," + String(rssi) + "," + String(detections),
        "uptime_ms,address,rssi,detections");
  }

 public:
  const char* title() const override { return "مراقب جهاز BLE"; }
  void onEnter() override {
    bleRx::begin();
    lastSeen = next = 0;
    detections = 0;
    low = high = rssi = -127;
    status = "رصد اعلانات فقط";
    busy = target[0] && bleRx::start(settings.bleSeconds);
    draw();
  }
  void onExit() override { bleRx::end(); }
  void update(uint32_t now) override {
    if (!target[0]) return;
    if (!busy) {
      if (now - next >= settings.refreshSeconds * 1000UL) {
        busy = bleRx::start(settings.bleSeconds);
        next = now;
      }
      return;
    }
    if (!bleRx::ready()) return;
    auto results = bleRx::results();
    bool found = false;
    for (int i = 0; i < results.getCount(); ++i) {
      auto d = results.getDevice(i);
      if (d.getAddress().toString() == std::string(target) && d.getRSSI() >= settings.minRssi) {
        rssi = d.getRSSI();
        if (!detections) low = high = rssi;
        low = min(low, rssi);
        high = max(high, rssi);
        lastSeen = now;
        detections++;
        found = true;
        break;
      }
    }
    status = found ? "الجهاز مرصود" : "لم يرصد في هذه الدورة";
    if (found && settings.autoSave && !save()) status = "فشل الحفظ SD";
    bleRx::clear();
    busy = false;
    next = now;
    draw();
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    int f = ui::footerHit(e.x, e.y, 3);
    if (f == 0) App::instance().open(&bleScanScreen);
    if (f == 1 && target[0] && !busy) {
      busy = bleRx::start(settings.bleSeconds);
      status = "جاري الرصد";
      draw();
    }
    if (f == 2) ui::toast(detections && save() ? "تم الحفظ" : "لا نتائج او فشل SD");
    return true;
  }
} tracker;
}  // namespace
void trackBle(const char* addr, const char* name) {
  strlcpy(target, addr, sizeof(target));
  strlcpy(label, name, sizeof(label));
}
Screen* bleTracker() { return &tracker; }
