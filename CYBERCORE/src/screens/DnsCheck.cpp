#include <WiFi.h>
#include <WiFiUdp.h>
#include "core/Protocol.h"
#include "core/Storage.h"
#include "screens/Screen.h"
#include "ui/Keyboard.h"
#include "ui/UI.h"
namespace {
class Dns : public Screen {
  WiFiUDP udp;
  char host[64] = "example.com";
  uint8_t query[300];
  size_t querySize = 0;
  IPAddress resolver, address;
  bool waiting = false, editing = false, hasResult = false;
  uint32_t began = 0, elapsed = 0;
  String message = "لا توجد نتيجة بعد";
  void draw() {
    ui::clearContent(true);
    txt::draw(160, 40, host, Theme::FG, Theme::BG, txt::CENTER, txt::FONT_M);
    String text = "DNS " + resolver.toString();
    txt::draw(160, 72, text.c_str(), Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
    txt::draw(160, 104, message.c_str(), Theme::FG, Theme::BG, txt::CENTER, txt::FONT_S);
    text = hasResult ? address.toString() + "  " + String(elapsed) + " ms" : "UDP A query";
    txt::draw(160, 137, text.c_str(), Theme::ACCENT, Theme::BG, txt::CENTER, txt::FONT_S);
    const char* f[] = {waiting ? "الغاء" : "اختبار", "تغيير الاسم", "حفظ SD"};
    ui::footerBar(f, nullptr, 3);
  }
  void start() {
    udp.stop();
    waiting = false;
    hasResult = false;
    if (WiFi.status() != WL_CONNECTED) {
      message = "اتصل بالواي فاي اولا";
      draw();
      return;
    }
    resolver = WiFi.dnsIP();
    querySize = protocol::dnsQuery(host, esp_random() & 65535, query, sizeof(query));
    if (!querySize || !uint32_t(resolver)) {
      message = "اسم او DNS غير صالح";
      draw();
      return;
    }
    if (!udp.begin(49152 + (esp_random() % 16384)) || !udp.beginPacket(resolver, 53) ||
        udp.write(query, querySize) != querySize || !udp.endPacket()) {
      udp.stop();
      message = "فشل ارسال الاستعلام";
      draw();
      return;
    }
    began = millis();
    waiting = true;
    message = "انتظار الرد - مهلة 3 ثوان";
    draw();
  }

 public:
  const char* title() const override { return "تشخيص DNS"; }
  void onEnter() override {
    editing = false;
    waiting = false;
    resolver = WiFi.dnsIP();
    draw();
  }
  void onExit() override {
    udp.stop();
    waiting = false;
  }
  void update(uint32_t now) override {
    if (!waiting) return;
    int size = udp.parsePacket();
    if (size > 0) {
      uint8_t data[512], ip[4];
      bool origin = udp.remoteIP() == resolver && udp.remotePort() == 53;
      int n = udp.read(data, sizeof(data));
      if (origin && size <= 512 && n == size) {
        int result = protocol::dnsAnswer(data, n, query, querySize, ip);
        if (result != 1) {
          waiting = false;
          elapsed = now - began;
          udp.stop();
          hasResult = result == 0;
          if (hasResult) {
            address = IPAddress(ip);
            message = "رد DNS مستلم";
          } else if (result == 19)
            message = "NXDOMAIN - الاسم غير موجود";
          else if (result == 2)
            message = "رد مختصر - يحتاج TCP";
          else if (result >= 16)
            message = "خطأ DNS " + String(result - 16);
          else
            message = "لا توجد اجابة IPv4";
          draw();
          return;
        }
      }
    }
    if (now - began >= 3000) {
      waiting = false;
      udp.stop();
      message = "انتهت المهلة - لا رد مطابق";
      draw();
    }
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    if (editing) {
      auto result = kb::tap(e.x, e.y);
      if (result == kb::DONE) {
        strlcpy(host, kb::text(), sizeof(host));
        editing = false;
        hasResult = false;
        message = "جاهز";
        draw();
      } else if (result == kb::CANCEL) {
        editing = false;
        draw();
      }
      return true;
    }
    int f = ui::footerHit(e.x, e.y, 3);
    if (f == 0) {
      if (waiting) {
        udp.stop();
        waiting = false;
        message = "الغي الاختبار";
        draw();
      } else
        start();
    }
    if (f == 1) {
      udp.stop();
      waiting = false;
      editing = true;
      kb::open("اسم النطاق", false, host);
    }
    if (f == 2) {
      bool ok = hasResult && storage::append("dns",
                                             String(millis()) + "," + storage::quote(host) + "," +
                                                 resolver.toString() + "," + address.toString() +
                                                 "," + String(elapsed),
                                             "uptime_ms,name,resolver,ipv4,elapsed_ms");
      ui::toast(ok ? "تم الحفظ" : "لا نتيجة او فشل SD");
    }
    return true;
  }
} dns;
}  // namespace
Screen* dnsCheck() { return &dns; }
