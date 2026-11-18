#include <WiFi.h>
#include "core/Settings.h"
#include "core/Storage.h"
#include <esp_wifi.h>
#include "screens/Modules.h"
#include "ui/UI.h"

// Passive air-traffic monitor: counts all 802.11 frames per channel and shows
// live activity. Never transmits.
namespace {
volatile uint32_t gPkt = 0;
const char* const kFoot[] = {"تصفير", "القناة", "وقف / تشغيل", "حفظ SD"};
constexpr int HN = 60;

void IRAM_ATTR cb(void* buf, wifi_promiscuous_pkt_type_t) {
  if (((wifi_promiscuous_pkt_t*)buf)->rx_ctrl.rssi >= settings.minRssi) gPkt++;
}
}  // namespace

void PacketMonitorScreen::onEnter() {
  ui::clearContent(true);
  paused_ = false;
  savedAt_ = millis();
  gPkt = 0;
  prev_ = 0;
  hi_ = 0;
  for (int i = 0; i < HN; i++) hist_[i] = 0;
  ch_ = settings.channel ? settings.channel : 1;
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(false);
  wifi_promiscuous_filter_t f = {};
  f.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
  esp_wifi_set_promiscuous_filter(&f);
  esp_wifi_set_promiscuous_rx_cb(&cb);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ch_, WIFI_SECOND_CHAN_NONE);
  lastHop_ = lastDraw_ = millis();
  draw();
  ui::footerBar(kFoot, nullptr, 4);
}

void PacketMonitorScreen::onExit() {
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_MODE_NULL);
}

void PacketMonitorScreen::update(uint32_t now) {
  if (!paused_ && !settings.channel && now - lastHop_ >= 250) {
    lastHop_ = now;
    ch_ = ch_ >= 13 ? 1 : ch_ + 1;
    esp_wifi_set_channel(ch_, WIFI_SECOND_CHAN_NONE);
  }
  if (settings.autoSave && now - savedAt_ >= settings.refreshSeconds * 1000UL) {
    savedAt_ = now;
    storage::append("traffic", String(now) + "," + String(ch_) + "," + String(gPkt),
                    "uptime_ms,channel,frames");
  }
  if (now - lastDraw_ >= 500) {
    lastDraw_ = now;
    const uint32_t d = gPkt - prev_;
    prev_ = gPkt;
    hist_[hi_] = (uint16_t)min<uint32_t>(d, 65535);
    hi_ = (hi_ + 1) % HN;
    draw();
  }
}

void PacketMonitorScreen::draw() {
  tft.fillRect(0, ui::CONTENT_Y, ui::W, ui::CONTENT_HB, Theme::BG);
  const uint32_t rate = hist_[(hi_ + HN - 1) % HN] * 2;  // per second (0.5s window)

  char n[16];
  snprintf(n, sizeof(n), "%lu", (unsigned long)rate);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(Theme::FG, Theme::BG);
  tft.drawString(n, ui::W / 2, ui::CONTENT_Y + 30, 7);
  txt::draw(ui::W / 2, ui::CONTENT_Y + 60, "اطار / ثانية", Theme::MID, Theme::BG, txt::CENTER,
            txt::FONT_S);

  char s[40];
  snprintf(s, sizeof(s), "الاجمالي %lu   قناة %d", (unsigned long)gPkt, ch_);
  txt::draw(ui::W / 2, ui::CONTENT_Y + 80, s, Theme::DIM, Theme::BG, txt::CENTER, txt::FONT_S);

  // activity graph
  const int16_t by = ui::FOOTER_Y - 8, bx = 10, bw = (ui::W - 20) / HN, h = 40;
  uint32_t mx = 1;
  for (int i = 0; i < HN; i++)
    if (hist_[i] > mx) mx = hist_[i];
  for (int i = 0; i < HN; i++) {
    const int idx = (hi_ + i) % HN;
    const int16_t bh = hist_[idx] ? (int16_t)(h * hist_[idx] / mx) : 0;
    tft.fillRect(bx + i * bw, by - bh, bw - 1, bh,
                 Theme::lerp(Theme::FG, Theme::WARN, bh * 255 / h));
  }
  tft.drawFastHLine(bx, by, ui::W - 20, Theme::GRID);
}

bool PacketMonitorScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  int action = ui::footerHit(e.x, e.y, 4);
  if (action == 1) {
    settings.channel = (settings.channel + 1) % 14;
    ch_ = settings.channel ? settings.channel : 1;
    esp_wifi_set_channel(ch_, WIFI_SECOND_CHAN_NONE);
    draw();
    return true;
  }
  if (action == 2) {
    paused_ = !paused_;
    esp_wifi_set_promiscuous(!paused_);
    ui::toast(paused_ ? "متوقف" : "رصد مستمر");
    return true;
  }
  if (action == 3) {
    bool ok = storage::append("traffic", String(millis()) + "," + String(ch_) + "," + String(gPkt),
                              "uptime_ms,channel,frames");
    ui::toast(ok ? "تم الحفظ" : "فشل الحفظ SD");
    return true;
  }
  if (action == 0) {
    gPkt = 0;
    prev_ = 0;
    for (int i = 0; i < HN; i++) hist_[i] = 0;
    draw();
    ui::footerBar(kFoot, nullptr, 4);
  }
  return true;
}
