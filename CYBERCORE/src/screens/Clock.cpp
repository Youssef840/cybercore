#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include "config.h"
#include "core/Settings.h"
#include "screens/Extras.h"
#include "ui/UI.h"
namespace {
bool ringing = false;
time_t lastAlarm = -1;
uint32_t ringAt = 0, lastTick = 0;
class Clock : public Screen {
  uint32_t last = 0;
  String message;
  void draw() {
    ui::clearContent(true);
    time_t now = time(nullptr);
    tm t;
    localtime_r(&now, &t);
    char b[64];
    strftime(b, sizeof(b), "%H:%M:%S", &t);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::FG, Theme::BG);
    tft.drawString(b, 160, 65, 4);
    txt::draw(160, 90, now < 1700000000 ? "اضبط الساعة او زامن الوقت" : "UTC+3", Theme::MID,
              Theme::BG, txt::CENTER, txt::FONT_S);
    const char* labels[] = {"الساعة +", "الدقيقة +", "المنبه + ساعة", "المنبه + دقيقة"};
    for (int i = 0; i < 4; ++i) {
      int x = 8 + (i % 2) * 156, y = 111 + (i / 2) * 29;
      ui::tile(x, y, 148, 27, false);
      ui::addFocus(x, y, 148, 27);
      txt::draw(x + 74, y + 5, labels[i], Theme::FG, Theme::TILE, txt::CENTER, txt::FONT_S);
    }
    snprintf(b, sizeof(b), "ALARM %02u:%02u %s", settings.alarmHour, settings.alarmMinute,
             settings.alarmEnabled ? "ON" : "OFF");
    txt::draw(160, 169, b, Theme::ACCENT, Theme::BG, txt::CENTER, txt::FONT_S);
    const char* f[] = {settings.alarmEnabled ? "تعطيل منبه" : "تفعيل منبه", "مزامنة NTP", "اسكات"};
    ui::footerBar(f, nullptr, 3);
  }

 public:
  const char* title() const override { return "ساعة ومنبه"; }
  void onEnter() override {
    setenv("TZ", "AST-3", 1);
    tzset();
    last = 0;
    draw();
  }
  void onExit() override { settings.save(); }
  void update(uint32_t now) override {
    if (now - last >= 1000) {
      last = now;
      draw();
    }
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    int f = ui::footerHit(e.x, e.y, 3);
    if (f == 0) {
      settings.alarmEnabled = !settings.alarmEnabled;
      settings.save();
      draw();
      return true;
    }
    if (f == 1) {
      if (WiFi.status() == WL_CONNECTED) {
        configTime(10800, 0, "pool.ntp.org", "time.google.com");
        ui::toast("طلب مزامنة الوقت");
      } else
        ui::toast("اتصل بالواي فاي اولا", Theme::WARN);
      return true;
    }
    if (f == 2) {
      dismissAlarm();
      return true;
    }
    if (e.y >= 111 && e.y < 169) {
      int i = (e.y - 111) / 29 * 2 + (e.x >= 164);
      if (i < 2) {
        time_t now = time(nullptr);
        if (now < 1700000000) now = 1794952800;
        timeval tv = {now + (i ? 60 : 3600), 0};
        settimeofday(&tv, nullptr);
      }
      if (i == 2) settings.alarmHour = (settings.alarmHour + 1) % 24;
      if (i == 3) settings.alarmMinute = (settings.alarmMinute + 1) % 60;
      draw();
      return true;
    }
    return false;
  }
} clockPage;
}  // namespace
Screen* clockScreen() { return &clockPage; }
bool dismissAlarm() {
  if (!ringing) return false;
  ringing = false;
  if (PIN_LED_G != PIN_NONE) digitalWrite(PIN_LED_G, HIGH);
  return true;
}
void clockTick(uint32_t now) {
  if (now - lastTick < 500) return;
  lastTick = now;
  time_t wall = time(nullptr);
  tm t;
  localtime_r(&wall, &t);
  time_t minute = wall / 60;
  if (settings.alarmEnabled && wall > 1700000000 && t.tm_hour == settings.alarmHour &&
      t.tm_min == settings.alarmMinute && minute != lastAlarm) {
    lastAlarm = minute;
    ringing = true;
    ringAt = now;
  }
  if (ringing) {
    if (now - ringAt > 60000) {
      dismissAlarm();
      return;
    }
    if (PIN_LED_G != PIN_NONE) digitalWrite(PIN_LED_G, (now / 500) % 2 ? LOW : HIGH);
    ui::toast("المنبه - اضغط للاسكات", Theme::WARN);
  }
}
