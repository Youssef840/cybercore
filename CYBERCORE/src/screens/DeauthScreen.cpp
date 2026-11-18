#include <WiFi.h>
#include "core/Settings.h"
#include "core/Storage.h"
#include <esp_wifi.h>
#include "screens/Modules.h"
#include "ui/UI.h"

// Passive monitor: counts 802.11 deauth/disassoc frames in the air and warns
// if your network is under a disconnect (deauth) attack. It never transmits.
namespace {
volatile uint32_t gDeauth = 0, gDisassoc = 0, gMgmt = 0;
uint32_t prevDeauth = 0;
const char* const kFoot[] = {"تصفير", "القناة", "وقف / تشغيل", "حفظ SD"};
constexpr int HN = 30;

void IRAM_ATTR promiscCb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  const wifi_promiscuous_pkt_t* p = (const wifi_promiscuous_pkt_t*)buf;
  if (p->rx_ctrl.sig_len < 24 || p->rx_ctrl.rssi < settings.minRssi) return;
  const uint8_t fc = p->payload[0];
  if (((fc >> 2) & 0x3) != 0) return;  // management frames only
  gMgmt++;
  const uint8_t sub = (fc >> 4) & 0xF;
  if (sub == 12)
    gDeauth++;
  else if (sub == 10)
    gDisassoc++;
}
}  // namespace

void DeauthScreen::onEnter() {
  ui::clearContent(true);
  paused_ = false;
  savedAt_ = millis();
  gDeauth = gDisassoc = gMgmt = 0;
  prevDeauth = 0;
  for (int i = 0; i < HN; i++) hist_[i] = 0;
  hi_ = 0;
  ch_ = settings.channel ? settings.channel : 1;

  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(false);
  wifi_promiscuous_filter_t f = {};
  f.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&f);
  esp_wifi_set_promiscuous_rx_cb(&promiscCb);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ch_, WIFI_SECOND_CHAN_NONE);

  lastHop_ = lastDraw_ = millis();
  draw();
  ui::footerBar(kFoot, nullptr, 4);
}

void DeauthScreen::onExit() {
  esp_wifi_set_promiscuous(false);
  WiFi.mode(WIFI_MODE_NULL);
}

void DeauthScreen::update(uint32_t now) {
  if (!paused_ && !settings.channel &&
      now - lastHop_ >= 300) {  // hop channels to watch the whole band
    lastHop_ = now;
    ch_ = ch_ >= 13 ? 1 : ch_ + 1;
    esp_wifi_set_channel(ch_, WIFI_SECOND_CHAN_NONE);
  }
  if (settings.autoSave && now - savedAt_ >= settings.refreshSeconds * 1000UL) {
    savedAt_ = now;
    storage::append(
        "deauth", String(now) + "," + String(ch_) + "," + String(gDeauth) + "," + String(gDisassoc),
        "uptime_ms,channel,deauth,disassoc");
  }
  if (now - lastDraw_ >= 500) {
    lastDraw_ = now;
    const uint32_t d = gDeauth - prevDeauth;
    prevDeauth = gDeauth;
    hist_[hi_] = d;
    hi_ = (hi_ + 1) % HN;
    draw();
  }
}

void DeauthScreen::draw() {
  const bool attack = hist_[(hi_ + HN - 1) % HN] >= settings.alertFrames;
  const uint16_t sc = attack ? Theme::ALERT : Theme::FG;
  tft.fillRect(0, ui::CONTENT_Y, ui::W, ui::CONTENT_HB, Theme::BG);

  // big deauth counter
  char n[12];
  snprintf(n, sizeof(n), "%lu", (unsigned long)gDeauth);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(sc, Theme::BG);
  tft.drawString(n, ui::W / 2, ui::CONTENT_Y + 36, 7);

  if (attack)
    icon::draw(tft, icon::ALERT, 40, ui::CONTENT_Y + 36, 16, Theme::ALERT);
  else
    icon::draw(tft, icon::SHIELD, 40, ui::CONTENT_Y + 36, 16, Theme::FG);

  txt::draw(ui::W / 2, ui::CONTENT_Y + 70,
            attack ? "ارتفاع اطارات الفصل" : "رصد سلبي - لا تأكيد لهجوم", sc, Theme::BG,
            txt::CENTER, txt::FONT_M);

  char s[48];
  snprintf(s, sizeof(s), "disassoc %lu   mgmt %lu   قناة %d", (unsigned long)gDisassoc,
           (unsigned long)gMgmt, ch_);
  txt::draw(ui::W / 2, ui::CONTENT_Y + 96, s, Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);

  // history bars
  const int16_t by = ui::FOOTER_Y - 6, bx = 16, bw = (ui::W - 32) / HN;
  uint32_t mx = 1;
  for (int i = 0; i < HN; i++)
    if (hist_[i] > mx) mx = hist_[i];
  for (int i = 0; i < HN; i++) {
    const int idx = (hi_ + i) % HN;
    const int16_t bh = hist_[idx] ? (int16_t)(26 * hist_[idx] / mx) : 0;
    tft.fillRect(bx + i * bw, by - bh, bw - 1, bh, hist_[idx] ? Theme::ALERT : Theme::GRID);
    if (!bh) tft.drawPixel(bx + i * bw, by - 1, Theme::GRID);
  }
}

bool DeauthScreen::onInput(const InputEvent& e) {
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
    bool ok = storage::append(
        "deauth",
        String(millis()) + "," + String(ch_) + "," + String(gDeauth) + "," + String(gDisassoc),
        "uptime_ms,channel,deauth,disassoc");
    ui::toast(ok ? "تم الحفظ" : "فشل الحفظ SD");
    return true;
  }
  if (action == 0) {
    gDeauth = gDisassoc = gMgmt = 0;
    prevDeauth = 0;
    for (int i = 0; i < HN; i++) hist_[i] = 0;
    draw();
    ui::footerBar(kFoot, nullptr, 4);
  }
  return true;
}
