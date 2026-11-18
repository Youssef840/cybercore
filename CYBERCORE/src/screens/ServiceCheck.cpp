#include <WiFi.h>
#include "core/NetworkScope.h"
#include "core/Storage.h"
#include "screens/Screen.h"
#include "ui/Keyboard.h"
#include "ui/UI.h"
namespace {
struct Port {
  uint16_t number;
  const char* name;
};
const Port ports[] = {{21, "FTP"},        {22, "SSH"},         {23, "TELNET"},     {53, "DNS TCP"},
                      {80, "HTTP"},       {139, "NETBIOS"},    {443, "HTTPS"},     {445, "SMB"},
                      {554, "RTSP"},      {1883, "MQTT"},      {3306, "MYSQL"},    {3389, "RDP"},
                      {8080, "HTTP ALT"}, {8443, "HTTPS ALT"}, {8883, "MQTT TLS"}, {9100, "PRINT"}};
class Services : public Screen {
  IPAddress target;
  uint16_t timeout = 250;
  int next = 0, top = 0;
  int8_t state[16] = {};
  bool running = false, editing = false;
  uint32_t last = 0;
  String message = "لا يعتبر البورت المفتوح ثغرة";
  bool local() { return netScope::localHost(target); }
  void draw() {
    ui::clearContent(true);
    String text = "IP: " + target.toString();
    ui::tile(8, 33, 304, 25, false);
    ui::addFocus(8, 33, 304, 25);
    txt::draw(16, 37, text.c_str(), Theme::FG, Theme::TILE, txt::LEFT, txt::FONT_S);
    ui::tile(8, 61, 304, 23, false);
    ui::addFocus(8, 61, 304, 23);
    text = String("Timeout ") + timeout + " ms   " + next + "/16";
    txt::draw(16, 64, text.c_str(), Theme::MID, Theme::TILE, txt::LEFT, txt::FONT_S);
    for (int j = 0; j < 4; ++j) {
      int i = top + j;
      if (i >= 16) break;
      char b[70];
      snprintf(b, sizeof(b), "%5u %-9s %s", ports[i].number, ports[i].name,
               state[i] > 0   ? "OPEN"
               : state[i] < 0 ? "NO REPLY"
                              : "--");
      txt::draw(10, 88 + j * 19, b, state[i] > 0 ? Theme::WARN : Theme::FG, Theme::BG, txt::LEFT,
                txt::FONT_S);
    }
    txt::draw(160, 166, message.c_str(), Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
    const char* f[] = {running ? "وقف" : "بدء", "اعلى", "اسفل", "حفظ SD"};
    ui::footerBar(f, nullptr, 4);
  }
  bool save() {
    if (!next) return false;
    for (int i = 0; i < next; ++i)
      if (!storage::append("services",
                           String(millis()) + "," + target.toString() + "," + ports[i].number +
                               "," + (state[i] > 0 ? "open" : "no_reply"),
                           "uptime_ms,target,port,result"))
        return false;
    return true;
  }

 public:
  const char* title() const override { return "فاحص الخدمات"; }
  void onEnter() override {
    if (!uint32_t(target)) target = WiFi.gatewayIP();
    editing = false;
    running = false;
    draw();
  }
  void onExit() override { running = false; }
  void update(uint32_t now) override {
    if (!running || now - last < 100) return;
    last = now;
    if (!local()) {
      running = false;
      message = "يلزم هدف داخل شبكتك المتصلة";
      draw();
      return;
    }
    WiFiClient client;
    state[next] = client.connect(target, ports[next].number, timeout) ? 1 : -1;
    client.stop();
    next++;
    if (next == 16) {
      running = false;
      message = "انتهى - عدم الرد لا يثبت الاغلاق";
    }
    draw();
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    if (editing) {
      auto result = kb::tap(e.x, e.y);
      if (result == kb::DONE) {
        IPAddress value;
        if (value.fromString(kb::text())) {
          target = value;
          next = 0;
          memset(state, 0, sizeof(state));
          message = "الهدف تغير";
        } else
          message = "عنوان IP غير صالح";
        editing = false;
        draw();
      } else if (result == kb::CANCEL) {
        editing = false;
        draw();
      }
      return true;
    }
    if (e.y >= 33 && e.y < 58) {
      running = false;
      editing = true;
      kb::open("عنوان جهازك", false, target.toString().c_str());
      return true;
    }
    if (e.y >= 61 && e.y < 84) {
      timeout = timeout == 100 ? 250 : timeout == 250 ? 500 : timeout == 500 ? 1000 : 100;
      draw();
      return true;
    }
    int f = ui::footerHit(e.x, e.y, 4);
    if (f == 0) {
      if (running)
        running = false;
      else if (local()) {
        next = 0;
        memset(state, 0, sizeof(state));
        running = true;
        message = "فحص TCP محدود";
      } else
        message = "اتصل بشبكتك واضبط الهدف";
      draw();
    }
    if (f == 1) {
      top = max(0, top - 4);
      draw();
    }
    if (f == 2) {
      top = min(12, top + 4);
      draw();
    }
    if (f == 3) ui::toast(save() ? "تم الحفظ" : "لا نتائج او فشل SD");
    return true;
  }
} services;
}  // namespace
Screen* serviceCheck() { return &services; }
