#pragma once
// ============================================================================
//  Module screens + the main icon grid. All labels Arabic, all WiFi features
//  passive/defensive (scan, audit) - no deauth, spam, or portal attacks.
// ============================================================================
#include "screens/Grid.h"

// ---- shared WiFi scan result -----------------------------------------------
struct ApInfo {
  char ssid[33];
  int8_t rssi;
  uint8_t channel;
  uint8_t enc;
  uint8_t bssid[6];
  bool hidden = false;
};
int wifiScanInto(ApInfo* out, int cap);
int wifiCollect(ApInfo* out, int cap);
bool saveWifiResults(const ApInfo* out, int n, const char* kind);
const char* encNameAr(uint8_t enc);

// ---------------------------------------------------------------- Wi-Fi scan -
class WifiScanScreen : public Screen {
 public:
  const char* title() const override { return "ماسح الشبكات"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void startScan();
  void drawList();
  void drawRow(int i);
  void footer();
  static constexpr int MAX = 32;
  static constexpr int VISIBLE = 5;
  ApInfo aps_[MAX];
  int n_ = 0, top_ = 0;
  bool scanning_ = false;
  uint32_t scannedAt_ = 0;
};

// --------------------------------------------------------------- Wi-Fi audit -
class WifiAuditScreen : public Screen {
 public:
  const char* title() const override { return "تدقيق الحماية"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void run();
  void draw();
  void drawRow(int i);
  void footer();
  static int riskOf(uint8_t enc);
  static constexpr int MAX = 32;
  static constexpr int VISIBLE = 3;
  ApInfo aps_[MAX];
  int n_ = 0, top_ = 0;
  int secure_ = 0, weak_ = 0, danger_ = 0;
  bool busy_ = false;
  uint32_t scannedAt_ = 0;
};

// --------------------------------------------------------------- System info -
class SysInfoScreen : public Screen {
 public:
  const char* title() const override { return "معلومات النظام"; }
  void onEnter() override;
  bool onInput(const InputEvent& e) override { return false; }
  void update(uint32_t now) override;

 private:
  uint32_t last_ = 0;
};

// ------------------------------------------------------------------ Settings -
class SettingsScreen : public Screen {
 public:
  const char* title() const override { return "الاعدادات"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void draw();
  void footer(int pressed = -1);
  uint32_t msg_ = 0;
};

// ----------------------------------------------------------- Channel graph -
class ChannelScreen : public Screen {
 public:
  const char* title() const override { return "محلل القنوات"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void run();
  void draw();
  uint8_t count_[15] = {0};
  int8_t best_[15];
  int busiest_ = 1, n_ = 0;
  bool busy_ = false;
  uint32_t scannedAt_ = 0;
};

// --------------------------------------------------------- Deauth detector -
class DeauthScreen : public Screen {
 public:
  const char* title() const override { return "كاشف الفصل"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void draw();
  uint32_t lastHop_ = 0, lastDraw_ = 0, hist_[32] = {0};
  uint8_t ch_ = 1, hi_ = 0;
  uint32_t shownDeauth_ = 0;
  bool paused_ = false;
  uint32_t savedAt_ = 0;
};

// -------------------------------------------------------------- BLE scanner -
class BleScanScreen : public Screen {
 public:
  const char* title() const override { return "ماسح بلوتوث"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  bool save();
  struct Dev {
    char name[24];
    char addr[18];
    int rssi;
  };
  void draw();
  void drawRow(int i);
  static constexpr int MAX = 24, VISIBLE = 5;
  Dev dev_[MAX];
  int n_ = 0, top_ = 0;
  bool scanning_ = false;
  uint32_t scannedAt_ = 0;
};

// -------------------------------------------------------------- I2C scanner -
class I2cScreen : public Screen {
 public:
  const char* title() const override { return "ماسح I2C"; }
  void onEnter() override;
  bool onInput(const InputEvent& e) override;

 private:
  void run();
  uint8_t found_[16];
  int n_ = 0;
};

// --------------------------------------------------------------- Flashlight -
class FlashlightScreen : public Screen {
 public:
  const char* title() const override { return "كشاف"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
};

// --------------------------------------------------------- Evil-twin detect -
class EvilTwinScreen : public Screen {
 public:
  const char* title() const override { return "كاشف التوأم"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void run();
  void draw();
  static constexpr int MAX = 40;
  ApInfo aps_[MAX];
  int n_ = 0, top_ = 0;
  int flags_[20], nflag_ = 0;  // indices of APs belonging to suspicious SSIDs
  bool busy_ = false;
  uint32_t scannedAt_ = 0;
};

// ------------------------------------------------------------ Packet monitor -
class PacketMonitorScreen : public Screen {
 public:
  const char* title() const override { return "محلل الحركة"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void draw();
  uint32_t lastHop_ = 0, lastDraw_ = 0, prev_ = 0;
  uint8_t ch_ = 1, hi_ = 0;
  uint16_t hist_[64] = {0};
  bool paused_ = false;
  uint32_t savedAt_ = 0;
};

// --------------------------------------------------------------- BLE radar ---
class BleRadarScreen : public Screen {
 public:
  const char* title() const override { return "رادار بلوتوث"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void draw(const char* name, const char* addr, int rssi, int count);
  uint32_t last_ = 0;
  float sweep_ = 0;
};

// ----------------------------------------------------------- ADC voltmeter ---
class AdcScreen : public Screen {
 public:
  const char* title() const override { return "مقياس جهد"; }
  void onEnter() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  uint32_t last_ = 0;
  uint8_t hi_ = 0;
  uint16_t hist_[80] = {0};
};

// --------------------------------------------------------------- Stopwatch ---
class StopwatchScreen : public Screen {
 public:
  const char* title() const override { return "مؤقت"; }
  void onEnter() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void draw();
  bool running_ = false;
  uint32_t start_ = 0, acc_ = 0, lastDraw_ = 0;
};

// -------------------------------------------------------------- Snake game ---
class SnakeScreen : public Screen {
 public:
  bool onNavigation(Btn button) override;
  const char* title() const override { return "افعى"; }
  void onEnter() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  void reset();
  void place();
  static constexpr int GW = 20, GH = 13;
  uint8_t sx_[GW * GH], sy_[GW * GH];
  int len_ = 0, dir_ = 0, fx_ = 0, fy_ = 0, score_ = 0;
  bool dead_ = false, paused_ = false;
  uint32_t last_ = 0;
};

// ------------------------------------------------------ Wi-Fi connect + net -
class NetToolsScreen : public Screen {
 public:
  const char* title() const override { return "اتصال وادوات"; }
  void onEnter() override;
  void onExit() override;
  bool onInput(const InputEvent& e) override;
  void update(uint32_t now) override;

 private:
  enum State : uint8_t { SCAN, LISTING, PASS, CONNECTING, INFO, FAIL, PING, LAN, PORTS };
  void startScan();
  void drawList();
  void drawRow(int i);
  void connect();
  void drawInfo();
  void startPing();
  void startLan();
  void startPorts();
  void drawLan(bool done);
  void drawPorts(bool done);
  static constexpr int MAX = 24, VISIBLE = 5;
  ApInfo aps_[MAX];
  int n_ = 0, top_ = 0;
  State st_ = SCAN;
  char ssid_[33] = "", pass_[64] = "";
  bool openNet_ = false;
  uint32_t t0_ = 0, workAt_ = 0;
  // LAN sweep
  int lanNext_ = 0, nfound_ = 0;
  uint8_t found_[48];
  // port scan
  int portNext_ = 0, nopen_ = 0;
  uint16_t openp_[24];
};

// ---------------------------------------------------------------- QR code ----
class QrScreen : public Screen {
 public:
  const char* title() const override { return "رمز QR"; }
  void onEnter() override;
  bool onInput(const InputEvent& e) override;

 private:
  void draw();
  char text_[64] = "CYBERCORE - WiFi/BLE Security Tool";
  bool editing_ = false;
};

extern WifiScanScreen wifiScanScreen;
extern WifiAuditScreen wifiAuditScreen;
extern ChannelScreen channelScreen;
extern DeauthScreen deauthScreen;
extern EvilTwinScreen evilTwinScreen;
extern PacketMonitorScreen packetMonitorScreen;
extern BleScanScreen bleScanScreen;
extern BleRadarScreen bleRadarScreen;
extern I2cScreen i2cScreen;
extern AdcScreen adcScreen;
extern FlashlightScreen flashlightScreen;
extern StopwatchScreen stopwatchScreen;
extern SnakeScreen snakeScreen;
extern QrScreen qrScreen;
extern NetToolsScreen netToolsScreen;
extern SysInfoScreen sysInfoScreen;
extern SettingsScreen settingsScreen;
extern Grid mainMenu, wifiGrid, bleGrid, hwGrid, toolsGrid;
