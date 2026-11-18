#include <WiFi.h>
#include <esp_wifi.h>
#include <stdarg.h>
#include "core/Settings.h"
#include "core/Storage.h"
#include "screens/Modules.h"
#include "ui/UI.h"
namespace {
struct Event {
  uint32_t at;
  char text[96];
};
class Watch : public Screen {
  ApInfo previous[32], current[32];
  int oldCount = 0, n = 0, head = 0, count = 0, top = 0;
  uint32_t sequence = 0, savedSequence = 0, finished = 0;
  Event events[32];
  bool baseline = false, busy = false, paused = false;
  String status;
  void log(const char* format, ...) {
    Event& e = events[head];
    e.at = millis();
    va_list args;
    va_start(args, format);
    vsnprintf(e.text, sizeof(e.text), format, args);
    va_end(args);
    head = (head + 1) % 32;
    count = min(32, count + 1);
    sequence++;
    top = 0;
  }
  String mac(const ApInfo& a) {
    char b[18];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", a.bssid[0], a.bssid[1], a.bssid[2],
             a.bssid[3], a.bssid[4], a.bssid[5]);
    return b;
  }
  bool save(bool onlyNew = false) {
    if (!count) return false;
    int amount = onlyNew ? min<uint32_t>(count, sequence - savedSequence) : count;
    if (!amount) return true;
    for (int j = amount - 1; j >= 0; --j) {
      auto& e = events[(head - 1 - j + 32) % 32];
      if (!storage::append("wifi_watch", String(e.at) + "," + storage::quote(e.text),
                           "uptime_ms,event"))
        return false;
    }
    savedSequence = sequence;
    return true;
  }
  void scan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanDelete();
    busy =
        WiFi.scanNetworks(true, true, true, settings.scanMs, settings.channel) == WIFI_SCAN_RUNNING;
    finished = millis();
    status = busy ? "مسح سلبي" : "فشل بدء المسح";
    draw();
  }
  void draw() {
    ui::clearContent(true);
    ui::tile(8, 33, 304, 25, false);
    ui::addFocus(8, 33, 304, 25);
    String line = "مرجع جديد | AP " + String(n) + " | events " + String(sequence);
    txt::draw(160, 37, line.c_str(), Theme::FG, Theme::TILE, txt::CENTER, txt::FONT_S);
    for (int j = 0; j < 4 && top + j < count; ++j) {
      auto& e = events[(head - 1 - top - j + 32) % 32];
      char time[20];
      snprintf(time, sizeof(time), "%lu s", (unsigned long)(e.at / 1000));
      txt::draw(8, 63 + j * 27, time, Theme::DIM, Theme::BG, txt::LEFT, txt::FONT_S);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(Theme::FG, Theme::BG);
      String text = e.text;
      while (tft.textWidth(text, 1) > 302 && text.length() > 3) text.remove(text.length() - 1);
      tft.drawString(text, 8, 76 + j * 27, 1);
    }
    txt::draw(160, 169, status.c_str(), Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
    const char* f[] = {paused ? "استئناف" : "وقف", "اعلى", "اسفل", "حفظ SD"};
    ui::footerBar(f, nullptr, 4);
  }

 public:
  const char* title() const override { return "مراقب تغير الشبكات"; }
  void onEnter() override {
    paused = false;
    top = 0;
    scan();
  }
  void onExit() override {
    esp_wifi_scan_stop();
    WiFi.scanDelete();
    busy = false;
    WiFi.mode(WIFI_MODE_NULL);
  }
  void update(uint32_t now) override {
    if (paused) return;
    if (!busy) {
      if (now - finished >= settings.refreshSeconds * 1000UL) scan();
      return;
    }
    int result = WiFi.scanComplete();
    if (result == WIFI_SCAN_RUNNING) return;
    busy = false;
    finished = now;
    if (result < 0) {
      status = "فشل المسح - المرجع محفوظ";
      draw();
      return;
    }
    n = wifiCollect(current, 32);
    if (!baseline) {
      memcpy(previous, current, n * sizeof(ApInfo));
      oldCount = n;
      baseline = true;
      log("BASELINE AP=%d", n);
    } else {
      for (int i = 0; i < n; ++i) {
        int old = -1;
        for (int j = 0; j < oldCount; ++j)
          if (!memcmp(current[i].bssid, previous[j].bssid, 6)) {
            old = j;
            break;
          }
        auto& a = current[i];
        String id = mac(a);
        if (old < 0) {
          log("NEW %s", id.c_str());
          continue;
        }
        auto& b = previous[old];
        if (a.enc != b.enc) log("AUTH %s %u->%u", id.c_str(), b.enc, a.enc);
        if (strcmp(a.ssid, b.ssid)) log("SSID changed %s", id.c_str());
        if (a.channel != b.channel) log("CHANNEL %s %u->%u", id.c_str(), b.channel, a.channel);
        if (abs(int(a.rssi) - int(b.rssi)) >= 20)
          log("SIGNAL %s %d->%d", id.c_str(), b.rssi, a.rssi);
      }
      for (int j = 0; j < oldCount; ++j) {
        bool seen = false;
        for (int i = 0; i < n; ++i)
          if (!memcmp(previous[j].bssid, current[i].bssid, 6)) {
            seen = true;
            break;
          }
        if (!seen) log("NOT SEEN %s", mac(previous[j]).c_str());
      }
      memcpy(previous, current, n * sizeof(ApInfo));
      oldCount = n;
    }
    status = "اختلاف الرصد لا يثبت هجوما";
    if (settings.autoSave && !save(true)) status = "فشل الحفظ SD";
    Serial.printf("[wifi-watch] aps=%d events=%lu\n", n, (unsigned long)sequence);
    draw();
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    if (e.y >= 33 && e.y < 58) {
      baseline = false;
      head = count = top = 0;
      sequence = savedSequence = 0;
      status = "سيؤخذ مرجع في المسح التالي";
      draw();
      return true;
    }
    int f = ui::footerHit(e.x, e.y, 4);
    if (f == 0) {
      paused = !paused;
      if (paused) {
        esp_wifi_scan_stop();
        WiFi.scanDelete();
        busy = false;
        status = "متوقف";
        draw();
      } else
        scan();
    }
    if (f == 1) {
      top = max(0, top - 4);
      draw();
    }
    if (f == 2) {
      top = min(max(0, count - 1), top + 4);
      draw();
    }
    if (f == 3) ui::toast(save() ? "تم الحفظ" : "لا نتائج او فشل SD");
    return true;
  }
} watch;
}  // namespace
Screen* wifiWatch() { return &watch; }
