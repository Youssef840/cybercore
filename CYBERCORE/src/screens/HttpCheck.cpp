#include <WiFi.h>
#include "core/NetworkScope.h"
#include "core/Protocol.h"
#include "core/Storage.h"
#include "screens/Screen.h"
#include "ui/Keyboard.h"
#include "ui/UI.h"
namespace {
class Http : public Screen {
  WiFiClient client;
  IPAddress target;
  uint16_t port = 80;
  protocol::HttpHeaders headers;
  bool editing = false, waiting = false, complete = false;
  uint32_t started = 0;
  size_t used = 0, total = 0;
  unsigned lines = 0;
  char line[256];
  int page = 0;
  String message = "HTTP HEAD / - بدون اتباع تحويلات";
  void stop(const char* why) {
    client.stop();
    waiting = false;
    message = why;
    draw();
  }
  void draw() {
    ui::clearContent(true);
    ui::tile(8, 33, 304, 25, false);
    ui::addFocus(8, 33, 304, 25);
    String text = target.toString() + ":" + port;
    txt::draw(160, 37, text.c_str(), Theme::FG, Theme::TILE, txt::CENTER, txt::FONT_S);
    text = "HTTP " + String(headers.code) + "  " + message;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(Theme::MID, Theme::BG);
    while (tft.textWidth(text, 1) > 304 && text.length()) text.remove(text.length() - 1);
    tft.drawString(text, 8, 63, 1);
    if (page == 0) {
      const char* names[] = {"CSP", "X-Frame-Options", "X-Content-Type", "Referrer-Policy",
                             "HSTS (HTTP only)"};
      for (int i = 0; i < 5; ++i) {
        text = String(names[i]) + ": " + ((headers.flags & (1 << i)) ? "SEEN" : "NOT SEEN");
        txt::draw(12, 82 + i * 18, text.c_str(), Theme::FG, Theme::BG, txt::LEFT, txt::FONT_S);
      }
    } else {
      txt::draw(12, 82, "SERVER", Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
      tft.drawString(headers.server, 12, 105, 1);
      txt::draw(12, 123, "LOCATION (not followed)", Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
      String value = headers.location;
      tft.drawString(value.substring(0, 47), 12, 146, 1);
      tft.drawString(value.substring(47), 12, 159, 1);
    }
    const char* f[] = {waiting ? "الغاء" : "فحص", "المنفذ", "تفاصيل", "حفظ SD"};
    ui::footerBar(f, nullptr, 4);
  }
  void start() {
    client.stop();
    complete = false;
    headers = protocol::HttpHeaders();
    page = 0;
    used = total = lines = 0;
    if (!netScope::localHost(target)) {
      stop("اتصل بشبكتك واضبط هدفا محليا");
      return;
    }
    if (!client.connect(target, port, 350)) {
      stop("لا اتصال TCP");
      return;
    }
    String request = "HEAD / HTTP/1.1\r\nHost: " + target.toString() + ":" + port +
                     "\r\nConnection: close\r\nUser-Agent: CYBERCORE\r\n\r\n";
    if (client.write(reinterpret_cast<const uint8_t*>(request.c_str()), request.length()) !=
        request.length()) {
      stop("تعذر ارسال HEAD");
      return;
    }
    started = millis();
    waiting = true;
    message = "انتظار 3 ثوان";
    draw();
  }

 public:
  const char* title() const override { return "فاحص رؤوس HTTP"; }
  void onEnter() override {
    if (!uint32_t(target)) target = WiFi.gatewayIP();
    editing = false;
    waiting = false;
    draw();
  }
  void onExit() override {
    client.stop();
    waiting = false;
  }
  void update(uint32_t now) override {
    if (!waiting) return;
    for (int budget = 0; budget < 512 && client.available(); ++budget) {
      char c = client.read();
      if ((uint8_t(c) < 32 && c != '\r' && c != '\n' && c != '\t') || uint8_t(c) == 127) {
        stop("محرف غير صالح في الرؤوس");
        return;
      }
      if (++total > 8192) {
        stop("تجاوز حد الرؤوس");
        return;
      }
      if (c == '\n') {
        if (used && line[used - 1] == '\r') used--;
        line[used] = 0;
        if (!used) {
          if (!headers.code || headers.code < 200) {
            stop("رد غير نهائي او غير صالح");
            return;
          }
          complete = true;
          Serial.printf("[http] response code=%u flags=%u\n", headers.code, headers.flags);
          stop("الرؤوس مكتملة - ليست نتيجة ثغرات");
          return;
        }
        if (++lines > 64 || (lines == 1 && strncmp(line, "HTTP/1.", 7)) ||
            !protocol::httpHeader(line, headers)) {
          stop("صيغة رؤوس غير مدعومة");
          return;
        }
        used = 0;
      } else {
        if (used >= sizeof(line) - 1) {
          stop("سطر طويل - اوقف الفحص");
          return;
        }
        line[used++] = c;
      }
    }
    if (now - started >= 3000)
      stop("مهلة - رؤوس غير مكتملة");
    else if (!client.connected() && !client.available())
      stop("انقطع الرد قبل اكتمال الرؤوس");
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    if (editing) {
      auto result = kb::tap(e.x, e.y);
      if (result == kb::DONE) {
        IPAddress value;
        if (value.fromString(kb::text())) {
          target = value;
          complete = false;
          headers = protocol::HttpHeaders();
          message = "الهدف تغير";
        } else
          message = "عنوان غير صالح";
        editing = false;
        draw();
      } else if (result == kb::CANCEL) {
        editing = false;
        draw();
      }
      return true;
    }
    if (e.y >= 33 && e.y < 58) {
      client.stop();
      waiting = false;
      editing = true;
      kb::open("عنوان جهازك", false, target.toString().c_str());
      return true;
    }
    int f = ui::footerHit(e.x, e.y, 4);
    if (f == 0) {
      if (waiting)
        stop("الغي الفحص");
      else
        start();
    }
    if (f == 1) {
      client.stop();
      waiting = false;
      port = port == 80 ? 8080 : port == 8080 ? 8000 : port == 8000 ? 8888 : 80;
      complete = false;
      headers = protocol::HttpHeaders();
      draw();
    }
    if (f == 2) {
      page = 1 - page;
      draw();
    }
    if (f == 3) {
      bool ok =
          complete && storage::append("http_headers",
                                      String(millis()) + "," + target.toString() + "," + port +
                                          "," + headers.code + "," + headers.flags + "," +
                                          storage::quote(headers.server) + "," +
                                          storage::quote(headers.location),
                                      "uptime_ms,target,port,status,seen_flags,server,location");
      ui::toast(ok ? "تم الحفظ" : "لا نتيجة او فشل SD");
    }
    return true;
  }
} http;
}  // namespace
Screen* httpCheck() { return &http; }
