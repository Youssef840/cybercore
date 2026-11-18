#include <WiFi.h>
#include <esp_wifi.h>
#include "core/Settings.h"
#include <ESP32Ping.h>
#include "screens/Modules.h"
#include "ui/Keyboard.h"
#include "ui/UI.h"

namespace {
constexpr int16_t LIST_Y = ui::CONTENT_Y + 2;
constexpr int16_t ROW_H = 30;
const char* const kFootScan[] = {"تحديث", "اعلى", "اسفل"};
const char* const kFootInfo[] = {"بوابة", "اجهزة", "بورت", "قطع"};
const char* const kFootBack[] = {"رجوع"};

struct Port {
  uint16_t port;
  const char* name;
};
const Port PORTS_[] = {{21, "FTP"},    {22, "SSH"},     {23, "Telnet"},  {53, "DNS"},
                       {80, "HTTP"},   {139, "SMB"},    {443, "HTTPS"},  {445, "SMB"},
                       {554, "RTSP"},  {1883, "MQTT"},  {3306, "MySQL"}, {3389, "RDP"},
                       {8080, "HTTP"}, {8443, "HTTPS"}, {8883, "MQTT"},  {9100, "Print"}};
constexpr int NPORTS = sizeof(PORTS_) / sizeof(PORTS_[0]);

bool tcpUp(IPAddress ip, uint16_t port, int to) {
  WiFiClient c;
  const bool ok = c.connect(ip, port, to);
  c.stop();
  return ok;
}
}  // namespace

void NetToolsScreen::onEnter() { startScan(); }
void NetToolsScreen::onExit() {
  if (st_ == SCAN) {
    esp_wifi_scan_stop();
    WiFi.scanDelete();
  }
}

void NetToolsScreen::startScan() {
  st_ = SCAN;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.scanDelete();
  WiFi.scanNetworks(true, true, true, settings.scanMs, settings.channel);
  ui::clearContent(true);
  ui::toast("جاري البحث...", Theme::WARN);
  ui::footerBar(kFootScan, nullptr, 3);
}

void NetToolsScreen::update(uint32_t now) {
  if (st_ == SCAN) {
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
    n_ = wifiCollect(aps_, MAX);
    int visible = 0;
    for (int i = 0; i < n_; ++i)
      if (!aps_[i].hidden) aps_[visible++] = aps_[i];
    n_ = visible;
    top_ = 0;
    st_ = LISTING;
    drawList();
  } else if (st_ == CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      st_ = INFO;
      drawInfo();
    } else if (now - t0_ > 15000) {
      st_ = FAIL;
      WiFi.disconnect();
      ui::clearContent(true);
      ui::toast("فشل الاتصال - تحقق من كلمة السر", Theme::ALERT);
      ui::footerBar(kFootScan, nullptr, 3);
    }
  } else if (st_ == LAN) {
    if (lanNext_ <= 254) {
      IPAddress me = WiFi.localIP();
      IPAddress t(me[0], me[1], me[2], lanNext_);
      if (tcpUp(t, 80, 90) || tcpUp(t, 443, 60))
        if (nfound_ < 48) found_[nfound_++] = lanNext_;
      lanNext_++;
      if (lanNext_ % 8 == 0 || lanNext_ > 254) drawLan(lanNext_ > 254);
    }
  } else if (st_ == PORTS) {
    if (portNext_ < NPORTS) {
      if (tcpUp(WiFi.gatewayIP(), PORTS_[portNext_].port, 250)) openp_[nopen_++] = portNext_;
      portNext_++;
      drawPorts(portNext_ >= NPORTS);
    }
  }
}

// ----------------------------------------------------------- scan listing ---
void NetToolsScreen::drawList() {
  ui::clearContent(true);
  if (n_ == 0)
    ui::toast("لا توجد شبكات", Theme::DIM);
  else
    for (int i = top_; i < top_ + VISIBLE && i < n_; i++) drawRow(i);
  ui::footerBar(kFootScan, nullptr, 3);
}

void NetToolsScreen::drawRow(int i) {
  const int16_t y = LIST_Y + (i - top_) * ROW_H;
  const ApInfo& a = aps_[i];
  ui::addFocus(0, y, ui::W, ROW_H - 1);
  const uint16_t sc = a.rssi > -70 ? Theme::FG : (a.rssi > -82 ? Theme::WARN : Theme::ALERT);
  tft.fillRect(0, y, ui::W, ROW_H - 1, Theme::BG);
  tft.drawFastHLine(8, y + ROW_H - 2, ui::W - 16, Theme::GRID);
  icon::draw(tft, icon::LOCK, 18, y + 13, 6, a.enc == WIFI_AUTH_OPEN ? Theme::ALERT : Theme::MID);
  char b[8];
  snprintf(b, sizeof(b), "%d", a.rssi);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(sc, Theme::BG);
  tft.drawString(b, 32, y + 14, 1);
  txt::draw(ui::W - 10, y + 3, a.ssid, Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_M);
}

// --------------------------------------------------------------- connect ----
void NetToolsScreen::connect() {
  st_ = CONNECTING;
  t0_ = millis();
  ui::clearContent(true);
  ui::toast("جاري الاتصال...", Theme::WARN);
  WiFi.begin(ssid_, openNet_ ? nullptr : pass_);
}

void NetToolsScreen::drawInfo() {
  st_ = INFO;
  ui::clearContent(true);
  txt::draw(ui::W - 10, ui::CONTENT_Y + 4, "متصل", Theme::FG, Theme::BG, txt::RIGHT, txt::FONT_M);
  icon::draw(tft, icon::SHIELD, 22, ui::CONTENT_Y + 14, 11, Theme::FG);
  struct {
    const char* l;
    String v;
  } rows[] = {{"الشبكة", WiFi.SSID()},
              {"IP", WiFi.localIP().toString()},
              {"البوابة", WiFi.gatewayIP().toString()},
              {"القناع", WiFi.subnetMask().toString()},
              {"DNS", WiFi.dnsIP().toString()},
              {"الاشارة", String(WiFi.RSSI()) + " dBm"},
              {"MAC", WiFi.macAddress()}};
  const int16_t y0 = ui::CONTENT_Y + 32, rh = 15;
  for (int i = 0; i < 7; i++) {
    const int16_t y = y0 + i * rh;
    txt::draw(ui::W - 10, y, rows[i].l, Theme::MID, Theme::BG, txt::RIGHT, txt::FONT_S);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(Theme::ACCENT, Theme::BG);
    tft.drawString(rows[i].v.c_str(), 12, y + 7, 2);
  }
  ui::footerBar(kFootInfo, nullptr, 4);
}

// ----------------------------------------------------------- gateway ping ---
void NetToolsScreen::startPing() {
  st_ = PING;
  ui::clearContent(true);
  txt::draw(ui::W - 10, ui::CONTENT_Y + 6, "اختبار البوابة", Theme::FG, Theme::BG, txt::RIGHT,
            txt::FONT_M);
  ui::toast("جاري الاختبار...", Theme::WARN);
  const IPAddress gw = WiFi.gatewayIP();
  const bool ok = Ping.ping(gw, 4);
  ui::clearContent(true);
  txt::draw(ui::W - 10, ui::CONTENT_Y + 6, "اختبار البوابة", Theme::FG, Theme::BG, txt::RIGHT,
            txt::FONT_M);
  tft.setTextDatum(MC_DATUM);
  if (ok) {
    char b[24];
    snprintf(b, sizeof(b), "%.0f ms", Ping.averageTime());
    tft.setTextColor(Theme::FG, Theme::BG);
    tft.drawString(b, ui::W / 2, ui::CONTENT_Y + 60, 7);
    txt::draw(ui::W / 2, ui::CONTENT_Y + 100, "البوابة تستجيب - متوسط الزمن", Theme::FG, Theme::BG,
              txt::CENTER, txt::FONT_S);
  } else {
    icon::draw(tft, icon::ALERT, ui::W / 2, ui::CONTENT_Y + 60, 20, Theme::ALERT);
    txt::draw(ui::W / 2, ui::CONTENT_Y + 100, "لا استجابة من البوابة", Theme::ALERT, Theme::BG,
              txt::CENTER, txt::FONT_M);
  }
  ui::footerBar(kFootBack, nullptr, 1);
}

// ------------------------------------------------------------- LAN sweep ----
void NetToolsScreen::startLan() {
  st_ = LAN;
  lanNext_ = 1;
  nfound_ = 0;
  drawLan(false);
}

void NetToolsScreen::drawLan(bool done) {
  ui::clearContent(true);
  char h[48];
  snprintf(h, sizeof(h), "فحص الاجهزة  %d/254", lanNext_ > 254 ? 254 : lanNext_);
  txt::draw(ui::W - 10, ui::CONTENT_Y + 4, h, done ? Theme::FG : Theme::WARN, Theme::BG, txt::RIGHT,
            txt::FONT_S);
  // progress bar
  const int16_t pw = (ui::W - 20) * (lanNext_ > 254 ? 254 : lanNext_) / 254;
  tft.drawRoundRect(10, ui::CONTENT_Y + 22, ui::W - 20, 8, 2, Theme::EDGE);
  tft.fillRect(12, ui::CONTENT_Y + 24, pw - 4 > 0 ? pw - 4 : 0, 4, Theme::FG);

  IPAddress me = WiFi.localIP();
  const int16_t gy = ui::CONTENT_Y + 36;
  for (int i = 0; i < nfound_ && i < 12; i++) {
    const int16_t x = 14 + (i % 2) * 150, y = gy + (i / 2) * 22;
    char ip[20];
    snprintf(ip, sizeof(ip), "%d.%d.%d.%d", me[0], me[1], me[2], found_[i]);
    icon::draw(tft, icon::CHIP, x + 6, y + 7, 6, Theme::ACCENT);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(Theme::FG, Theme::BG);
    tft.drawString(ip, x + 18, y + 7, 2);
  }
  char c[32];
  snprintf(c, sizeof(c), done ? "اكتمل - %d جهاز" : "", nfound_);
  if (done)
    txt::draw(ui::W / 2, ui::FOOTER_Y - 16, c, Theme::ACCENT, Theme::BG, txt::CENTER, txt::FONT_S);
  ui::footerBar(kFootBack, nullptr, 1);
}

// ------------------------------------------------------------- port scan ----
void NetToolsScreen::startPorts() {
  st_ = PORTS;
  portNext_ = 0;
  nopen_ = 0;
  drawPorts(false);
}

void NetToolsScreen::drawPorts(bool done) {
  ui::clearContent(true);
  char h[48];
  snprintf(h, sizeof(h), "بورتات البوابة  %d/%d", portNext_, NPORTS);
  txt::draw(ui::W - 10, ui::CONTENT_Y + 4, h, done ? Theme::FG : Theme::WARN, Theme::BG, txt::RIGHT,
            txt::FONT_S);
  const int16_t gy = ui::CONTENT_Y + 28;
  if (nopen_ == 0 && done)
    txt::draw(ui::W / 2, gy + 30, "لا يوجد بورت مفتوح", Theme::DIM, Theme::BG, txt::CENTER,
              txt::FONT_M);
  for (int i = 0; i < nopen_ && i < 12; i++) {
    const int16_t x = 16 + (i % 2) * 152, y = gy + (i / 2) * 24;
    char b[24];
    snprintf(b, sizeof(b), "%u  %s", PORTS_[openp_[i]].port, PORTS_[openp_[i]].name);
    icon::draw(tft, icon::LOCK, x + 6, y + 7, 5, Theme::WARN);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(Theme::FG, Theme::BG);
    tft.drawString(b, x + 18, y + 8, 2);
  }
  ui::footerBar(kFootBack, nullptr, 1);
}

// ----------------------------------------------------------------- input ----
bool NetToolsScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;

  if (st_ == PASS) {
    const kb::Result r = kb::tap(e.x, e.y);
    if (r == kb::DONE) {
      strncpy(pass_, kb::text(), sizeof(pass_) - 1);
      connect();
    } else if (r == kb::CANCEL)
      startScan();
    return true;
  }
  if (st_ == LISTING) {
    int f = ui::footerHit(e.x, e.y, 3);
    if (f == 0) {
      startScan();
      return true;
    }
    if (f == 1) {
      top_ = max(0, top_ - VISIBLE);
      drawList();
      return true;
    }
    if (f == 2) {
      if (top_ + VISIBLE < n_) top_ += VISIBLE;
      drawList();
      return true;
    }
    if (e.y >= LIST_Y && e.y < LIST_Y + VISIBLE * ROW_H) {
      const int i = top_ + (e.y - LIST_Y) / ROW_H;
      if (i < n_) {
        strncpy(ssid_, aps_[i].ssid, sizeof(ssid_) - 1);
        openNet_ = aps_[i].enc == WIFI_AUTH_OPEN;
        if (openNet_)
          connect();
        else {
          st_ = PASS;
          kb::open("كلمة سر الشبكة", true);
        }
      }
    }
    return true;
  }
  if (st_ == INFO) {
    const int f = ui::footerHit(e.x, e.y, 4);
    if (f == 0)
      startPing();
    else if (f == 1)
      startLan();
    else if (f == 2)
      startPorts();
    else if (f == 3) {
      WiFi.disconnect();
      startScan();
    }
    return true;
  }
  if (st_ == PING || st_ == LAN || st_ == PORTS) {
    if (ui::footerHit(e.x, e.y, 1) == 0) drawInfo();  // back to info
    return true;
  }
  if (st_ == FAIL) {
    startScan();
    return true;
  }
  return true;
}
