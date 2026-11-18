#include "core/Settings.h"
#include "screens/Screen.h"
#include "ui/UI.h"
namespace {
class Options : public Screen {
  int page = 0;
  void draw() {
    ui::clearContent(true);
    const char* labels[] = {
        "مدة قناة WiFi", "القناة - صفر تلقائي", "اقل اشارة dBm", "مدة مسح BLE",
        "تحديث تلقائي",  "فترة التحديث",        "حفظ تلقائي SD", "عتبة الفصل لكل نصف ثانية"};
    for (int j = 0; j < 4; ++j) {
      int i = page * 4 + j, y = ui::CONTENT_Y + j * 34;
      String v;
      switch (i) {
        case 0:
          v = String(settings.scanMs) + " ms";
          break;
        case 1:
          v = String(settings.channel);
          break;
        case 2:
          v = String(settings.minRssi);
          break;
        case 3:
          v = String(settings.bleSeconds) + " s";
          break;
        case 4:
          v = settings.autoRefresh ? "ON" : "OFF";
          break;
        case 5:
          v = String(settings.refreshSeconds) + " s";
          break;
        case 6:
          v = settings.autoSave ? "ON" : "OFF";
          break;
        case 7:
          v = String(settings.alertFrames);
          break;
      }
      ui::tile(8, y, 304, 31, false);
      ui::addFocus(8, y, 304, 31);
      txt::draw(300, y + 7, labels[i], Theme::FG, Theme::TILE, txt::RIGHT, txt::FONT_S);
      txt::draw(16, y + 7, v.c_str(), Theme::ACCENT, Theme::TILE, txt::LEFT, txt::FONT_S);
    }
    const char* f[] = {"السابق", "التالي", "حفظ"};
    ui::footerBar(f, nullptr, 3);
  }

 public:
  const char* title() const override { return "خيارات الادوات"; }
  void onEnter() override { draw(); }
  void onExit() override { settings.save(); }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    int f = ui::footerHit(e.x, e.y, 3);
    if (f == 0 || f == 1) {
      page = 1 - page;
      draw();
      return true;
    }
    if (f == 2) {
      settings.save();
      draw();
      return true;
    }
    if (e.y < ui::CONTENT_Y || e.y >= ui::CONTENT_Y + 136) return false;
    int i = page * 4 + (e.y - ui::CONTENT_Y) / 34;
    switch (i) {
      case 0:
        settings.scanMs = settings.scanMs >= 1000 ? 100 : settings.scanMs + 100;
        break;
      case 1:
        settings.channel = (settings.channel + 1) % 14;
        break;
      case 2:
        settings.minRssi = settings.minRssi >= -30 ? -110 : settings.minRssi + 10;
        break;
      case 3:
        settings.bleSeconds = settings.bleSeconds % 10 + 1;
        break;
      case 4:
        settings.autoRefresh = !settings.autoRefresh;
        break;
      case 5:
        settings.refreshSeconds = settings.refreshSeconds >= 120 ? 5 : settings.refreshSeconds + 5;
        break;
      case 6:
        settings.autoSave = !settings.autoSave;
        break;
      case 7:
        settings.alertFrames = settings.alertFrames >= 50 ? 1
                               : settings.alertFrames < 5 ? 5
                                                          : settings.alertFrames + 5;
        break;
    }
    draw();
    return true;
  }
} options;
}  // namespace
Screen* toolOptions() { return &options; }
