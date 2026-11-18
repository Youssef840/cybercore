#include <WiFi.h>
#include <esp_wifi.h>
#include <atomic>
#include "core/Protocol.h"
#include "core/Settings.h"
#include "core/Storage.h"
#include "screens/Screen.h"
#include "ui/UI.h"
namespace {
struct Record {
  protocol::Advertisement ad;
  uint8_t mac[6];
  int8_t rssi;
  uint8_t channel;
};
Record records[32];
int count = 0;
std::atomic<bool> active(false);
portMUX_TYPE guard = portMUX_INITIALIZER_UNLOCKED;
void receive(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!active.load() || type != WIFI_PKT_MGMT) return;
  auto p = static_cast<wifi_promiscuous_pkt_t*>(buf);
  size_t n = p->rx_ctrl.sig_len;
  if (n < 40 || n > 2304 || p->rx_ctrl.rssi < settings.minRssi) return;
  uint8_t subtype = p->payload[0] & 0xfc;
  if (subtype != 0x80 && subtype != 0x50) return;
  protocol::Advertisement ad;
  if (!protocol::advertisement(p->payload + 36, n - 40, ad)) return;
  portENTER_CRITICAL(&guard);
  int index = -1;
  for (int i = 0; i < count; ++i)
    if (!memcmp(records[i].mac, p->payload + 16, 6)) {
      index = i;
      break;
    }
  if (index < 0 && count < 32) index = count++;
  if (index >= 0) {
    auto& r = records[index];
    r.ad = ad;
    memcpy(r.mac, p->payload + 16, 6);
    r.rssi = p->rx_ctrl.rssi;
    r.channel = p->rx_ctrl.channel;
  }
  portEXIT_CRITICAL(&guard);
}
const char* pmfName(uint8_t value) {
  return value == 3 ? "REQUIRED" : value == 2 ? "CAPABLE" : value == 1 ? "NOT SET" : "UNKNOWN";
}
class Audit : public Screen {
  Record copy[32];
  int n = 0, top = 0, selected = -1;
  bool paused = false;
  uint8_t channel = 1;
  uint32_t lastHop = 0, lastDraw = 0;
  String mac(const Record& r) {
    char b[18];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", r.mac[0], r.mac[1], r.mac[2], r.mac[3],
             r.mac[4], r.mac[5]);
    return b;
  }
  void snapshot() {
    portENTER_CRITICAL(&guard);
    n = count;
    memcpy(copy, records, n * sizeof(Record));
    portEXIT_CRITICAL(&guard);
  }
  bool save() {
    snapshot();
    if (!n) return false;
    for (int i = 0; i < n; ++i) {
      auto& r = copy[i];
      if (!storage::append("beacon_audit",
                           String(millis()) + "," + storage::quote(r.ad.ssid) + "," + mac(r) + "," +
                               String(r.channel) + "," + String(r.rssi) + "," + pmfName(r.ad.pmf) +
                               "," + String(r.ad.wps),
                           "uptime_ms,ssid,bssid,channel,rssi,pmf,wps"))
        return false;
    }
    return true;
  }
  void draw() {
    ui::clearContent(true);
    char b[96];
    if (selected >= 0 && selected < n) {
      auto& r = copy[selected];
      txt::draw(160, 38, r.ad.ssid[0] ? r.ad.ssid : "<hidden>", Theme::FG, Theme::BG, txt::CENTER,
                txt::FONT_M);
      txt::draw(160, 67, mac(r).c_str(), Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
      snprintf(b, sizeof(b), "C%u  RSSI %d  PMF %s", r.channel, r.rssi, pmfName(r.ad.pmf));
      txt::draw(160, 98, b, Theme::FG, Theme::BG, txt::CENTER, txt::FONT_S);
      txt::draw(160, 125, r.ad.wps ? "WPS advertised" : "WPS not observed", Theme::MID, Theme::BG,
                txt::CENTER, txt::FONT_S);
      txt::draw(160, 153, "اعلان فقط - لا يثبت ثغرة", Theme::MID, Theme::BG, txt::CENTER,
                txt::FONT_S);
      const char* f[] = {"القائمة", "حفظ SD"};
      ui::footerBar(f, nullptr, 2);
      return;
    }
    snprintf(b, sizeof(b), "AP %d   CH %u   %s", n, channel, paused ? "PAUSED" : "PASSIVE");
    txt::draw(8, 33, b, Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
    for (int j = 0; j < 3 && top + j < n; ++j) {
      auto& r = copy[top + j];
      int y = 52 + j * 43;
      ui::tile(6, y, 308, 40, false);
      ui::addFocus(6, y, 308, 40);
      txt::draw(304, y + 2, r.ad.ssid[0] ? r.ad.ssid : "<hidden>", Theme::FG, Theme::TILE,
                txt::RIGHT, txt::FONT_S);
      snprintf(b, sizeof(b), "C%u %d  PMF:%s  WPS:%s", r.channel, r.rssi, pmfName(r.ad.pmf),
               r.ad.wps ? "YES" : "--");
      txt::draw(12, y + 21, b, Theme::MID, Theme::TILE, txt::LEFT, txt::FONT_S);
    }
    const char* f[] = {"اعلى", "اسفل", paused ? "استئناف" : "وقف", "حفظ SD"};
    ui::footerBar(f, nullptr, 4);
  }

 public:
  const char* title() const override { return "تدقيق PMF و WPS"; }
  void onEnter() override {
    portENTER_CRITICAL(&guard);
    count = 0;
    portEXIT_CRITICAL(&guard);
    n = top = 0;
    selected = -1;
    paused = false;
    channel = settings.channel ? settings.channel : 1;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(false);
    wifi_promiscuous_filter_t f = {};
    f.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    esp_wifi_set_promiscuous_filter(&f);
    esp_wifi_set_promiscuous_rx_cb(receive);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    active = true;
    esp_wifi_set_promiscuous(true);
    lastHop = lastDraw = millis();
    draw();
  }
  void onExit() override {
    active = false;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    WiFi.mode(WIFI_MODE_NULL);
  }
  void update(uint32_t now) override {
    if (!paused && !settings.channel && now - lastHop >= settings.scanMs) {
      lastHop = now;
      channel = channel >= 13 ? 1 : channel + 1;
      esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
    if (selected < 0 && now - lastDraw >= 1000) {
      lastDraw = now;
      snapshot();
      Serial.printf("[beacon-audit] observed=%d\n", n);
      draw();
    }
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    if (selected >= 0) {
      int f = ui::footerHit(e.x, e.y, 2);
      if (f == 0) {
        selected = -1;
        draw();
      }
      if (f == 1) ui::toast(save() ? "تم الحفظ" : "فشل الحفظ SD");
      return true;
    }
    int f = ui::footerHit(e.x, e.y, 4);
    if (f == 0) {
      top = max(0, top - 3);
      draw();
    }
    if (f == 1) {
      if (top + 3 < n) top += 3;
      draw();
    }
    if (f == 2) {
      paused = !paused;
      active = !paused;
      esp_wifi_set_promiscuous(!paused);
      draw();
    }
    if (f == 3) ui::toast(save() ? "تم الحفظ" : "فشل الحفظ SD");
    if (e.y >= 52 && e.y < 181) {
      int i = top + (e.y - 52) / 43;
      if (i < n) {
        selected = i;
        draw();
      }
    }
    return true;
  }
} audit;
}  // namespace
Screen* beaconAudit() { return &audit; }
