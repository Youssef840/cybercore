#include "core/BleReceiver.h"
#include "core/Settings.h"
#include "core/Storage.h"
#include "screens/Screen.h"
#include "ui/UI.h"
namespace {
struct Device {
  char name[25], addr[18], uuid[38], manufacturer[33], payload[49];
  int rssi, tx;
  uint16_t company, length;
  uint8_t type, services;
  bool hasCompany, hasTx;
};
void hex(char* out, const uint8_t* bytes, size_t length, size_t max) {
  static const char* d = "0123456789ABCDEF";
  size_t n = min(length, max);
  for (size_t i = 0; i < n; ++i) {
    out[2 * i] = d[bytes[i] >> 4];
    out[2 * i + 1] = d[bytes[i] & 15];
  }
  out[2 * n] = 0;
}
class Inspector : public Screen {
  Device devices[16];
  int n = 0, top = 0, selected = -1, page = 0;
  bool busy = false;
  uint32_t finished = 0;
  bool save() {
    if (!n) return false;
    for (int i = 0; i < n; ++i) {
      auto& d = devices[i];
      String line = String(millis()) + "," + storage::quote(d.name) + "," + d.addr + "," +
                    String(d.rssi) + "," + String(d.type) + "," + storage::quote(d.uuid) + "," +
                    d.manufacturer + "," + d.payload;
      if (!storage::append("ble_ads", line,
                           "uptime_ms,name,address,rssi,address_type,first_uuid,manufacturer_"
                           "prefix_hex,payload_prefix_hex"))
        return false;
    }
    return true;
  }
  void scan() {
    busy = bleRx::start(settings.bleSeconds);
    finished = millis();
    draw();
  }
  void draw() {
    ui::clearContent(true);
    char b[100];
    if (selected >= 0 && selected < n) {
      auto& d = devices[selected];
      txt::draw(160, 36, d.name, Theme::FG, Theme::BG, txt::CENTER, txt::FONT_M);
      txt::draw(160, 62, d.addr, Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
      if (page == 0) {
        snprintf(b, sizeof(b), "RSSI %d  ADDR TYPE %u", d.rssi, d.type);
        txt::draw(12, 88, b, Theme::FG, Theme::BG, txt::LEFT, txt::FONT_S);
        snprintf(b, sizeof(b), "TX %s  Services %u", d.hasTx ? String(d.tx).c_str() : "unknown",
                 d.services);
        txt::draw(12, 113, b, Theme::FG, Theme::BG, txt::LEFT, txt::FONT_S);
        snprintf(b, sizeof(b), "Company ID: %s",
                 d.hasCompany ? String(d.company, HEX).c_str() : "unknown");
        txt::draw(12, 139, b, Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
      }
      if (page == 1) {
        txt::draw(12, 87, "FIRST SERVICE UUID", Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(Theme::FG, Theme::BG);
        tft.drawString(d.uuid, 12, 108, 1);
        txt::draw(12, 129, "MANUFACTURER PREFIX", Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
        tft.drawString(d.manufacturer, 12, 151, 1);
      }
      if (page == 2) {
        snprintf(b, sizeof(b), "PAYLOAD %u bytes / prefix 24", d.length);
        txt::draw(12, 88, b, Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
        String payload = d.payload;
        txt::draw(12, 116, payload.substring(0, 24).c_str(), Theme::FG, Theme::BG, txt::LEFT,
                  txt::FONT_S);
        txt::draw(12, 140, payload.substring(24).c_str(), Theme::FG, Theme::BG, txt::LEFT,
                  txt::FONT_S);
      }
      const char* f[] = {"قائمة", "السابق", "التالي", "حفظ SD"};
      ui::footerBar(f, nullptr, 4);
      return;
    }
    snprintf(b, sizeof(b), "DEV %d   %s", n, busy ? "PASSIVE SCAN" : "READY");
    txt::draw(8, 33, b, Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
    for (int j = 0; j < 4 && top + j < n; ++j) {
      int y = 53 + j * 31;
      auto& d = devices[top + j];
      ui::tile(8, y, 304, 29, false);
      if (!busy) ui::addFocus(8, y, 304, 29);
      txt::draw(300, y + 3, d.name, Theme::FG, Theme::TILE, txt::RIGHT, txt::FONT_S);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(Theme::MID, Theme::TILE);
      snprintf(b, sizeof(b), "%s  %d dBm", d.addr, d.rssi);
      tft.drawString(b, 14, y + 19, 1);
    }
    const char* f[] = {"مسح", "اعلى", "اسفل", "حفظ SD"};
    ui::footerBar(f, nullptr, 4);
  }

 public:
  const char* title() const override { return "محلل اعلانات BLE"; }
  void onEnter() override {
    bleRx::begin();
    selected = -1;
    top = 0;
    scan();
  }
  void onExit() override {
    bleRx::end();
    busy = false;
  }
  void update(uint32_t now) override {
    if (selected >= 0) return;
    if (!busy) {
      if (settings.autoRefresh && now - finished >= settings.refreshSeconds * 1000UL) scan();
      return;
    }
    if (!bleRx::ready()) return;
    auto results = bleRx::results();
    n = 0;
    for (int i = 0; i < results.getCount() && n < 16; ++i) {
      auto d = results.getDevice(i);
      if (d.getRSSI() < settings.minRssi) continue;
      auto& v = devices[n++];
      v = Device();
      strlcpy(v.name, d.haveName() ? d.getName().c_str() : "<unnamed>", sizeof(v.name));
      strlcpy(v.addr, d.getAddress().toString().c_str(), sizeof(v.addr));
      v.rssi = d.getRSSI();
      v.type = d.getAddressType();
      v.services = min(255, d.getServiceUUIDCount());
      if (d.haveServiceUUID())
        strlcpy(v.uuid, d.getServiceUUID().toString().c_str(), sizeof(v.uuid));
      auto data = d.getManufacturerData();
      v.hasCompany = data.size() >= 2;
      if (v.hasCompany) v.company = uint8_t(data[0]) | (uint16_t(uint8_t(data[1])) << 8);
      hex(v.manufacturer, reinterpret_cast<const uint8_t*>(data.data()), data.size(), 16);
      v.hasTx = d.haveTXPower();
      if (v.hasTx) v.tx = d.getTXPower();
      v.length = d.getPayloadLength();
      hex(v.payload, d.getPayload(), v.length, 24);
    }
    bleRx::clear();
    busy = false;
    finished = now;
    top = 0;
    if (settings.autoSave) save();
    Serial.printf("[ble-inspector] devices=%d\n", n);
    draw();
  }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    if (selected >= 0) {
      int f = ui::footerHit(e.x, e.y, 4);
      if (f == 0) {
        selected = -1;
        finished = millis();
        draw();
      }
      if (f == 1 || f == 2) {
        page = (page + (f == 1 ? 2 : 1)) % 3;
        draw();
      }
      if (f == 3) ui::toast(save() ? "تم الحفظ" : "فشل الحفظ SD");
      return true;
    }
    int f = ui::footerHit(e.x, e.y, 4);
    if (f == 0 && !busy) scan();
    if (f == 1) {
      top = max(0, top - 4);
      draw();
    }
    if (f == 2) {
      if (top + 4 < n) top += 4;
      draw();
    }
    if (f == 3) ui::toast(save() ? "تم الحفظ" : "لا نتائج او فشل SD");
    if (!busy && e.y >= 53 && e.y < 177) {
      int i = top + (e.y - 53) / 31;
      if (i < n) {
        selected = i;
        page = 0;
        draw();
      }
    }
    return true;
  }
} inspector;
}  // namespace
Screen* bleInspector() { return &inspector; }
