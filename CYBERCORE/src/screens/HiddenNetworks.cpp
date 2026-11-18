#include <WiFi.h>
#include <esp_wifi.h>
#include "core/Settings.h"
#include "screens/Modules.h"
#include "ui/UI.h"
namespace {
class Hidden : public Screen {
  ApInfo aps[40];
  int n = 0, top = 0;
  bool busy = false;
  uint32_t done = 0;
  void draw() {
    ui::clearContent(true);
    if (!n)
      txt::draw(160, 85, busy ? "جاري المسح السلبي" : "لا توجد شبكات مخفية مرصودة", Theme::MID,
                Theme::BG, txt::CENTER, txt::FONT_S);
    for (int j = 0; j < 4 && top + j < n; ++j) {
      auto& a = aps[top + j];
      char b[64];
      snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X   C%u   %d", a.bssid[0], a.bssid[1],
               a.bssid[2], a.bssid[3], a.bssid[4], a.bssid[5], a.channel, a.rssi);
      txt::draw(10, 42 + j * 31, b, Theme::FG, Theme::BG, txt::LEFT, txt::FONT_S);
    }
    const char* f[] = {"مسح", "اعلى", "اسفل", "حفظ SD"};
    ui::footerBar(f, nullptr, 4);
  }
  void scan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanDelete();
    busy =
        WiFi.scanNetworks(true, true, true, settings.scanMs, settings.channel) == WIFI_SCAN_RUNNING;
    done = millis();
    draw();
  }

 public:
  const char* title() const override { return "شبكات مخفية"; }
  void onEnter() override {
    n = top = 0;
    scan();
  }
  void onExit() override {
    esp_wifi_scan_stop();
    WiFi.scanDelete();
    WiFi.mode(WIFI_MODE_NULL);
  }
  void update(uint32_t now) override {
    if (!busy) {
      if (settings.autoRefresh && now - done >= settings.refreshSeconds * 1000UL) scan();
      return;
    }
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
    n = wifiCollect(aps, 40);
    int k = 0;
    for (int i = 0; i < n; ++i)
      if (aps[i].hidden) aps[k++] = aps[i];
    n = k;
    top = 0;
    busy = false;
    done = millis();
    if (settings.autoSave && n) saveWifiResults(aps, n, "hidden");
    draw();
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    int f = ui::footerHit(e.x, e.y, 4);
    if (f == 0 && !busy) scan();
    if (f == 1) {
      top = max(0, top - 4);
      draw();
    }
    if (f == 2 && top + 4 < n) {
      top += 4;
      draw();
    }
    if (f == 3) ui::toast(saveWifiResults(aps, n, "hidden") ? "تم الحفظ" : "فشل الحفظ SD");
    return true;
  }
} hidden;
}  // namespace
Screen* hiddenNetworks() { return &hidden; }
