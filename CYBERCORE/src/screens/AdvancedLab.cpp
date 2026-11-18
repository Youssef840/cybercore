#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include <SD.h>
#include <sys/time.h>
#include <mbedtls/sha256.h>
#include "core/LabCodec.h"
#include "core/NetworkScope.h"
#include "core/Protocol.h"
#include "core/Settings.h"
#include "core/Storage.h"
#include "screens/Modules.h"
#include "ui/Keyboard.h"
#include "ui/UI.h"
namespace {
struct Description {
  const char* title;
  const char* labels[3];
  const char* defaults[3];
};
const Description descriptions[26] = {
    {"جودة اتصال WiFi", {"عدد العينات", nullptr, nullptr}, {"30", "", ""}},
    {"اقتراح قناة WiFi", {nullptr, nullptr, nullptr}, {"", "", ""}},
    {"زمن اتصال TCP", {"الهدف", "المنفذ", "عدد المحاولات"}, {"gateway", "80", "5"}},
    {"استمرارية HTTP", {"الهدف", "المنفذ", "عدد المحاولات"}, {"gateway", "80", "5"}},
    {"تعريف الخدمة", {"الهدف", "المنفذ", nullptr}, {"gateway", "22", ""}},
    {"اختبار وقت NTP", {"خادم IP", nullptr, nullptr}, {"gateway", "", ""}},
    {"سجلات AAAA", {"النطاق", "خادم DNS", nullptr}, {"example.com", "auto", ""}},
    {"سجلات MX", {"النطاق", "خادم DNS", nullptr}, {"example.com", "auto", ""}},
    {"سجلات TXT", {"النطاق", "خادم DNS", nullptr}, {"example.com", "auto", ""}},
    {"سجلات NS", {"النطاق", "خادم DNS", nullptr}, {"example.com", "auto", ""}},
    {"بحث DNS عكسي", {"عنوان IPv4", "خادم DNS", nullptr}, {"192.168.1.1", "auto", ""}},
    {"مقارنة DNS", {"النطاق", "الخادم الاول", "الخادم الثاني"}, {"example.com", "auto", "auto2"}},
    {"استقرار DNS", {"النطاق", "خادم DNS", "عدد المحاولات"}, {"example.com", "auto", "5"}},
    {"حاسبة الشبكة", {"IPv4/CIDR", nullptr, nullptr}, {"192.168.1.10/24", "", ""}},
    {"تصنيف IPv4", {"عنوان IPv4", nullptr, nullptr}, {"192.168.1.10", "", ""}},
    {"تداخل الشبكات", {"شبكة A", "شبكة B", nullptr}, {"192.168.1.0/24", "192.168.1.128/25", ""}},
    {"تنسيق IPv6", {"عنوان IPv6", nullptr, nullptr}, {"2001:db8::1", "", ""}},
    {"تحليل عنوان MAC", {"عنوان MAC", nullptr, nullptr}, {"02:00:00:00:00:01", "", ""}},
    {"توليد كلمة مرور", {"الطول 12 الى 48", nullptr, nullptr}, {"24", "", ""}},
    {"تحويل Base64", {"النص", "encode / decode", nullptr}, {"CYBERCORE", "encode", ""}},
    {"تحويل Hex", {"النص", "encode / decode", nullptr}, {"CYBERCORE", "encode", ""}},
    {"ترميز URL", {"النص", "encode / decode", nullptr}, {"hello world", "encode", ""}},
    {"بصمة نص SHA256", {"النص", nullptr, nullptr}, {"CYBERCORE", "", ""}},
    {"بصمة ملف SHA256", {"مسار ملف SD", nullptr, nullptr}, {"/CYBERCORE/wifi.csv", "", ""}},
    {"توليد UUID", {nullptr, nullptr, nullptr}, {"", "", ""}},
    {"زمن نقل ملف", {"الحجم MiB", "السرعة Mbps", nullptr}, {"100", "20", ""}},
};
class Runner {
  enum State { Idle, Scan, WifiSeries, Delay, TcpBanner, Http, Dns, Ntp, Hash };
  State state = Idle;
  uint8_t id = 0;
  String input[3], output[40];
  int n = 0, page = 0, editing = -1;
  bool hasResult = false, ownRadio = false, hashActive = false;
  WiFiClient client;
  WiFiUDP udp;
  File file;
  mbedtls_sha256_context hashContext;
  IPAddress target, server, secondServer;
  uint16_t port = 80, dnsType = 1;
  uint8_t query[300], nonce[8];
  size_t querySize = 0;
  lab::DnsRecords records;
  String compareAnswers[2];
  int compareCode[2] = {};
  unsigned comparison = 0;
  uint32_t began = 0, due = 0, totalTime = 0, best = UINT32_MAX, worst = 0;
  double localSent = 0;
  unsigned attempts = 0, limit = 5, successes = 0, disconnects = 0;
  int lowest = -127, highest = -127;
  int32_t signalSum = 0;
  unsigned signalCount = 0;
  String lastBssid;
  unsigned bssidChanges = 0;
  struct Sample {
    uint32_t ms;
    uint16_t code;
    bool ok;
  } samples[120];
  char banner[256];
  size_t used = 0;
  uint32_t lastPaint = 0;
  int fields() const {
    int total = 0;
    while (total < 3 && descriptions[id].labels[total]) total++;
    return total;
  }
  int visible() const { return max(1, (173 - (35 + fields() * 26)) / 17); }
  void clear() {
    for (auto& line : output) line = "";
    n = page = 0;
    hasResult = false;
  }
  void add(String value) {
    if (!value.length() && n < 40) output[n++] = "(empty)";
    while (value.length() && n < 40) {
      output[n++] = value.substring(0, 46);
      value.remove(0, min<unsigned>(46, value.length()));
    }
  }
  void report(bool ok) {
    hasResult = ok;
    Serial.printf("[lab-result] id=%u ok=%u lines=%d\n", id, ok, n);
    draw();
  }
  void fail(const char* why) {
    cancel(false);
    clear();
    add(why);
    report(false);
  }
  bool integer(const String& value, unsigned lo, unsigned hi, unsigned& out) {
    if (!value.length()) return false;
    unsigned long v = 0;
    for (unsigned i = 0; i < value.length(); ++i) {
      if (value[i] < '0' || value[i] > '9' || v > 1000000) return false;
      v = v * 10 + value[i] - '0';
    }
    if (v < lo || v > hi) return false;
    out = v;
    return true;
  }
  bool resolve(const String& value, IPAddress& ip) {
    if (value == "gateway")
      ip = WiFi.gatewayIP();
    else if (value == "auto")
      ip = WiFi.dnsIP(0);
    else if (value == "auto2")
      ip = WiFi.dnsIP(1);
    else if (!ip.fromString(value))
      return false;
    return uint32_t(ip) && ip[0] < 224;
  }
  void cancel(bool release) {
    client.stop();
    udp.stop();
    file.close();
    if (hashActive) {
      mbedtls_sha256_free(&hashContext);
      hashActive = false;
    }
    if (state == Scan) {
      esp_wifi_scan_stop();
      WiFi.scanDelete();
    }
    state = Idle;
    if (release && ownRadio) {
      WiFi.mode(WIFI_MODE_NULL);
      ownRadio = false;
    }
  }
  void statistics() {
    clear();
    add("Target " + target.toString() + ":" + port);
    add("Attempts " + String(attempts) + " / " + limit);
    add("Success " + String(successes) + " / failed " + String(attempts - successes));
    if (successes) {
      add("ms min/avg/max: " + String(best) + " / " + String(double(totalTime) / successes, 1) +
          " / " + String(worst));
    }
    for (unsigned i = 0; i < attempts && i < 120; ++i)
      add(String(i + 1) + ": " + (id == 3 ? "HTTP " + String(samples[i].code) + " " : "") +
          (samples[i].ok ? "OK " : "FAIL ") + String(samples[i].ms) + " ms");
    add(id == 2 ? "TCP connect time; not ICMP ping" : "HTTP 2xx/3xx = success; no redirects");
    hasResult = attempts > 0;
  }
  void finishProbe(bool ok, uint16_t code, uint32_t elapsed) {
    client.stop();
    samples[attempts] = {elapsed, code, ok};
    attempts++;
    if (ok) {
      successes++;
      totalTime += elapsed;
      best = min(best, elapsed);
      worst = max(worst, elapsed);
    }
    statistics();
    state = attempts >= limit ? Idle : Delay;
    due = millis() + (id == 3 ? 2000 : 1000);
    if (state == Idle)
      report(true);
    else
      draw();
  }
  void probe() {
    if (!netScope::localHost(target)) {
      fail("Target must be a host in connected LAN");
      return;
    }
    began = millis();
    bool connected = client.connect(target, port, 250);
    if (id == 2) {
      finishProbe(connected, 0, millis() - began);
      return;
    }
    if (!connected) {
      finishProbe(false, 0, millis() - began);
      return;
    }
    String request = "HEAD / HTTP/1.1\r\nHost: " + target.toString() + ":" + port +
                     "\r\nConnection: close\r\nUser-Agent: CYBERCORE\r\n\r\n";
    if (client.write(reinterpret_cast<const uint8_t*>(request.c_str()), request.length()) !=
        request.length()) {
      finishProbe(false, 0, millis() - began);
      return;
    }
    used = 0;
    state = Http;
  }
  String domain() {
    if (id != 10) return input[0];
    uint32_t ip;
    if (!lab::ipv4(input[0], ip)) return "";
    return String(uint8_t(ip)) + "." + uint8_t(ip >> 8) + "." + uint8_t(ip >> 16) + "." +
           uint8_t(ip >> 24) + ".in-addr.arpa";
  }
  void sendDns() {
    udp.stop();
    String host = domain();
    querySize = protocol::dnsQuery(host.c_str(), esp_random() & 65535, query, sizeof(query));
    if (!querySize) {
      fail("Invalid hostname or IPv4 input");
      return;
    }
    query[querySize - 4] = dnsType >> 8;
    query[querySize - 3] = dnsType;
    if (!udp.begin(49152 + (esp_random() % 16384)) || !udp.beginPacket(server, 53) ||
        udp.write(query, querySize) != querySize || !udp.endPacket()) {
      fail("DNS socket/send failed");
      return;
    }
    state = Dns;
    began = millis();
  }
  void dnsDone(int result, uint32_t elapsed) {
    udp.stop();
    if (id == 11) {
      compareCode[comparison] = result;
      String joined;
      String values[8];
      for (int i = 0; i < records.count; ++i) values[i] = records.value[i];
      for (int i = 0; i < records.count; ++i)
        for (int j = i + 1; j < records.count; ++j)
          if (values[j] < values[i]) {
            String tmp = values[i];
            values[i] = values[j];
            values[j] = tmp;
          }
      for (int i = 0; i < records.count; ++i) {
        joined += values[i];
        joined += ';';
      }
      compareAnswers[comparison] = joined;
      if (comparison == 0) {
        comparison = 1;
        server = secondServer;
        sendDns();
        return;
      }
      clear();
      add("Resolver 1 " + target.toString() + ": " + String(compareCode[0]));
      add(compareAnswers[0]);
      add("Resolver 2 " + secondServer.toString() + ": " + String(compareCode[1]));
      add(compareAnswers[1]);
      if (!compareCode[0] && !compareCode[1])
        add(compareAnswers[0] == compareAnswers[1] ? "SAME returned A set"
                                                   : "DIFFERENT returned A sets");
      else
        add("A answers could not be compared");
      add("CDNs/cache may legitimately differ");
      state = Idle;
      report(true);
      return;
    }
    if (id == 12) {
      bool replied = result != -1;
      samples[attempts] = {elapsed, uint16_t(result < 0 ? 65535 : result), replied};
      attempts++;
      if (replied) {
        successes++;
        totalTime += elapsed;
        best = min(best, elapsed);
        worst = max(worst, elapsed);
      }
      clear();
      add("DNS " + server.toString());
      add("Queries " + String(attempts) + " / " + limit);
      add("Matching replies " + String(successes));
      if (successes)
        add("ms min/avg/max: " + String(best) + " / " + String(double(totalTime) / successes, 1) +
            " / " + String(worst));
      for (unsigned i = 0; i < attempts; ++i)
        add(String(i + 1) + ": status " +
            (samples[i].code == 65535 ? String("TIMEOUT") : String(samples[i].code)) + " / " +
            samples[i].ms + " ms");
      add("Matching errors also count as replies");
      state = attempts >= limit ? Idle : Delay;
      due = millis() + 1000;
      if (state == Idle)
        report(true);
      else
        draw();
      return;
    }
    clear();
    add("DNS " + server.toString() + " / " + elapsed + " ms");
    add("Status " + String(result));
    if (result == 0) {
      for (unsigned i = 0; i < records.count; ++i) add(records.value[i]);
    } else if (result == -1)
      add("No matching response before timeout");
    else if (result == 2)
      add("Truncated UDP; TCP not attempted");
    else if (result == 3)
      add("No requested record in answer section");
    else if (result >= 16)
      add("DNS RCODE " + String(result - 16));
    state = Idle;
    report(result != -1);
  }
  void quality(uint32_t now) {
    if (int32_t(now - due) < 0) return;
    due = now + 1000;
    attempts++;
    if (WiFi.status() == WL_CONNECTED) {
      int rssi = WiFi.RSSI();
      if (!signalCount) lowest = highest = rssi;
      lowest = min(lowest, rssi);
      highest = max(highest, rssi);
      signalSum += rssi;
      signalCount++;
      String bssid = WiFi.BSSIDstr();
      if (lastBssid.length() && lastBssid != bssid) bssidChanges++;
      lastBssid = bssid;
    } else
      disconnects++;
    clear();
    add("Samples " + String(attempts) + " / " + limit);
    add("Connected samples: " + String(signalCount));
    add("Disconnected samples: " + String(disconnects));
    if (signalCount)
      add("RSSI min/avg/max: " + String(lowest) + " / " +
          String(double(signalSum) / signalCount, 1) + " / " + String(highest));
    add("BSSID changes: " + String(bssidChanges));
    if (lastBssid.length()) add(lastBssid);
    add("RSSI is not a throughput measurement");
    if (attempts >= limit) {
      state = Idle;
      report(true);
    } else
      draw();
  }

 public:
  const char* title(unsigned which) const { return descriptions[which].title; }
  void enter(unsigned which) {
    cancel(true);
    id = which;
    for (int i = 0; i < 3; ++i) input[i] = descriptions[id].defaults[i];
    clear();
    editing = -1;
    add("اضبط المدخلات ثم اضغط بدء");
    draw();
  }
  void leave() {
    cancel(true);
    if (id == 18) clear();
  }
  void draw() {
    ui::clearContent(true);
    int f = fields();
    for (int i = 0; i < f; ++i) {
      int y = 33 + i * 26;
      ui::tile(8, y, 304, 24, false);
      ui::addFocus(8, y, 304, 24);
      txt::draw(304, y + 4, descriptions[id].labels[i], Theme::MID, Theme::TILE, txt::RIGHT,
                txt::FONT_S);
      String value = input[i];
      if (value.length() > 25) value = value.substring(0, 22) + "...";
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(Theme::FG, Theme::TILE);
      tft.drawString(value, 13, y + 7, 1);
    }
    int rows = visible(), start = page * rows;
    for (int i = 0; i < rows && start + i < n; ++i) {
      int y = 35 + f * 26 + i * 17;
      const String& line = output[start + i];
      if (txt::width(line.c_str(), txt::FONT_S) > 304) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(Theme::FG, Theme::BG);
        tft.drawString(line, 8, y + 4, 1);
      } else
        txt::draw(8, y, line.c_str(), Theme::FG, Theme::BG, txt::LEFT, txt::FONT_S);
    }
    char hint[60];
    snprintf(hint, sizeof(hint), "LAB %02u/26   page %d/%d   %s", id + 1, page + 1,
             max(1, (n + rows - 1) / rows), state == Idle ? "READY" : "RUNNING");
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(Theme::DIM, Theme::BG);
    tft.drawString(hint, 8, 175, 1);
    const char* labels[] = {state == Idle ? "بدء" : "وقف", "السابق", "التالي", "حفظ SD"};
    ui::footerBar(labels, nullptr, 4);
  }
  bool inputEvent(const InputEvent& e) {
    if (!e.tap()) return false;
    if (editing >= 0) {
      auto result = kb::tap(e.x, e.y);
      if (result == kb::DONE) {
        input[editing] = kb::text();
        editing = -1;
        clear();
        add("المدخل تغير - ابدأ من جديد");
        draw();
      } else if (result == kb::CANCEL) {
        editing = -1;
        draw();
      }
      return true;
    }
    int f = fields();
    if (e.y >= 33 && e.y < 33 + f * 26) {
      cancel(false);
      editing = (e.y - 33) / 26;
      kb::open(descriptions[id].labels[editing], false, input[editing].c_str());
      return true;
    }
    int action = ui::footerHit(e.x, e.y, 4);
    if (action == 0) {
      if (state != Idle) {
        cancel(false);
        add("STOPPED");
        draw();
      } else
        start();
    }
    int pages = max(1, (n + visible() - 1) / visible());
    if (action == 1 || action == 2) {
      page = (page + (action == 1 ? pages - 1 : 1)) % pages;
      draw();
    }
    if (action == 3) {
      if (id == 18) {
        ui::toast("حفظ كلمات المرور معطل", Theme::WARN);
        return true;
      }
      bool ok = hasResult && n > 0;
      const uint32_t exportId = millis();
      for (int i = 0; ok && i < n; ++i)
        ok = storage::append("advanced_lab",
                             String(exportId) + "," + String(id) + "," + storage::quote(title(id)) +
                                 "," + storage::quote(input[0].c_str()) + "," +
                                 storage::quote(input[1].c_str()) + "," +
                                 storage::quote(input[2].c_str()) + "," + String(i) + "," +
                                 storage::quote(output[i].c_str()),
                             "export_uptime_ms,tool_id,tool,input_1,input_2,input_3,line,result");
      ui::toast(ok ? "تم الحفظ" : "لا نتيجة او فشل SD");
    }
    return true;
  }
  void start() {
    cancel(true);
    clear();
    attempts = successes = disconnects = signalCount = bssidChanges = 0;
    totalTime = 0;
    best = UINT32_MAX;
    worst = 0;
    signalSum = 0;
    lastBssid = "";
    lastPaint = 0;
    if (id >= 13 && id != 23) {
      bool ok = lab::calculate(id, input, output, n);
      report(ok);
      return;
    }
    if (id == 23) {
      if (!storage::mount()) {
        fail(storage::lastError());
        return;
      }
      String path = input[0];
      if (!path.startsWith("/")) path = "/" + path;
      file = SD.open(path, FILE_READ);
      if (!file || file.isDirectory()) {
        fail("File not found or path is directory");
        return;
      }
      mbedtls_sha256_init(&hashContext);
      hashActive = true;
      if (mbedtls_sha256_starts_ret(&hashContext, 0)) {
        fail("SHA256 initialization failed");
        return;
      }
      state = Hash;
      add("Hashing file in chunks...");
      draw();
      return;
    }
    if (id == 1) {
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      WiFi.scanDelete();
      ownRadio = true;
      if (WiFi.scanNetworks(true, true, true, settings.scanMs, 0) != WIFI_SCAN_RUNNING) {
        fail("Could not start passive survey");
        return;
      }
      state = Scan;
      add("Passive survey of all channels...");
      draw();
      return;
    }
    if (WiFi.status() != WL_CONNECTED) {
      fail("Connect WiFi with connection tools first");
      return;
    }
    if (id == 0) {
      if (!integer(input[0], 5, 120, limit)) {
        fail("Samples must be 5..120");
        return;
      }
      state = WifiSeries;
      due = millis();
      add("Sampling link once per second...");
      draw();
      return;
    }
    if (id >= 2 && id <= 4) {
      unsigned parsed;
      if (!resolve(input[0], target) || !netScope::localHost(target) ||
          !integer(input[1], 1, 65535, parsed)) {
        fail("Invalid local host or TCP port");
        return;
      }
      port = parsed;
      if (id < 4) {
        if (!integer(input[2], 1, 20, limit)) {
          fail("Attempts must be 1..20");
          return;
        }
        probe();
        draw();
        return;
      }
      began = millis();
      if (!client.connect(target, port, 250)) {
        fail("TCP connection failed");
        return;
      }
      used = 0;
      state = TcpBanner;
      add("Waiting for server-first banner...");
      draw();
      return;
    }
    if (id == 5) {
      if (!resolve(input[0], server)) {
        fail("Invalid NTP server IPv4");
        return;
      }
      uint8_t packet[48] = {};
      packet[0] = 0x23;
      esp_fill_random(nonce, sizeof(nonce));
      memcpy(packet + 40, nonce, 8);
      timeval wall;
      gettimeofday(&wall, nullptr);
      localSent = double(wall.tv_sec) * 1000 + wall.tv_usec / 1000.0;
      if (!udp.begin(49152 + (esp_random() % 16384)) || !udp.beginPacket(server, 123) ||
          udp.write(packet, sizeof(packet)) != sizeof(packet) || !udp.endPacket()) {
        fail("NTP socket/send failed");
        return;
      }
      began = millis();
      state = Ntp;
      add("Waiting for one NTP response...");
      draw();
      return;
    }
    if (id >= 6 && id <= 12) {
      if (!resolve(input[1], server)) {
        fail("Invalid DNS resolver IPv4");
        return;
      }
      dnsType = id == 6 ? 28 : id == 7 ? 15 : id == 8 ? 16 : id == 9 ? 2 : id == 10 ? 12 : 1;
      if (id == 11) {
        if (!resolve(input[2], secondServer) || secondServer == server) {
          fail("Provide two different DNS resolver IPs");
          return;
        }
        comparison = 0;
        target = server;
        compareAnswers[0] = compareAnswers[1] = "";
        compareCode[0] = compareCode[1] = -1;
      }
      if (id == 12 && !integer(input[2], 1, 20, limit)) {
        fail("Queries must be 1..20");
        return;
      }
      sendDns();
      if (state == Dns) {
        add("Waiting for matching DNS response...");
        draw();
      }
      return;
    }
  }
  void update(uint32_t now) {
    if (editing >= 0) return;
    if (state == Scan) {
      int status = WiFi.scanComplete();
      if (status == WIFI_SCAN_RUNNING) return;
      if (status < 0) {
        fail("Passive survey failed");
        return;
      }
      static ApInfo aps[64];
      int count = wifiCollect(aps, 64);
      clear();
      if (!count) {
        add("No APs passed the scan/RSSI filter");
        state = Idle;
        report(false);
        return;
      }
      int score[14] = {};
      for (int c = 1; c <= 13; ++c)
        for (int i = 0; i < count; ++i) {
          int distance = abs(c - int(aps[i].channel));
          if (distance < 5) score[c] += max(1, 100 + int(aps[i].rssi)) * (5 - distance);
        }
      int bestChannel = 1;
      for (int c : {6, 11})
        if (score[c] < score[bestChannel]) bestChannel = c;
      add("Suggested 1/6/11: channel " + String(bestChannel));
      add("AP count: " + String(count));
      for (int c = 1; c <= 13; ++c) add("CH " + String(c) + " overlap score " + String(score[c]));
      add("AP-based estimate, not RF noise level");
      state = Idle;
      report(true);
      return;
    }
    if (state == WifiSeries) {
      quality(now);
      return;
    }
    if (state == Delay && int32_t(now - due) >= 0) {
      if (id == 12)
        sendDns();
      else
        probe();
      return;
    }
    if (state == TcpBanner) {
      for (int budget = 0; budget < 128 && client.available(); ++budget) {
        char c = client.read();
        if (c == '\n' || used >= sizeof(banner) - 1) {
          banner[used] = 0;
          client.stop();
          state = Idle;
          clear();
          add("Server-first banner (no login):");
          add(banner);
          report(true);
          return;
        }
        if (c != '\r') banner[used++] = (uint8_t(c) >= 32 && uint8_t(c) < 127) ? c : '?';
      }
      if (now - began >= 2000 || (!client.connected() && !client.available())) {
        banner[used] = 0;
        client.stop();
        state = Idle;
        clear();
        add(used ? "Partial banner:" : "No server-first banner within 2 s");
        if (used) add(banner);
        report(used > 0);
      }
      return;
    }
    if (state == Http) {
      for (int budget = 0; budget < 128 && client.available(); ++budget) {
        char c = client.read();
        if (c == '\n') {
          if (used && banner[used - 1] == '\r') used--;
          banner[used] = 0;
          protocol::HttpHeaders h;
          bool valid = protocol::httpHeader(banner, h);
          finishProbe(valid && h.code >= 200 && h.code < 400, valid ? h.code : 0, millis() - began);
          return;
        }
        if ((uint8_t(c) < 32 && c != '\r' && c != '\t') || uint8_t(c) == 127 ||
            used >= sizeof(banner) - 1) {
          finishProbe(false, 0, millis() - began);
          return;
        }
        banner[used++] = c;
      }
      if (now - began >= 2500 || (!client.connected() && !client.available()))
        finishProbe(false, 0, millis() - began);
      return;
    }
    if (state == Dns) {
      int size = udp.parsePacket();
      if (size > 0) {
        uint8_t packet[512];
        bool origin = udp.remoteIP() == server && udp.remotePort() == 53;
        int got = udp.read(packet, sizeof(packet));
        if (origin && size <= 512 && got == size) {
          int result = lab::dnsRecords(packet, got, query, querySize, dnsType, records);
          if (result != 1) {
            dnsDone(result, now - began);
            return;
          }
        }
      }
      if (now - began >= 3000) {
        records = lab::DnsRecords();
        dnsDone(-1, now - began);
      }
      return;
    }
    if (state == Ntp) {
      int size = udp.parsePacket();
      if (size > 0) {
        uint8_t packet[512];
        bool origin = udp.remoteIP() == server && udp.remotePort() == 123;
        int got = udp.read(packet, sizeof(packet));
        if (origin && size <= 512 && got == size && got >= 48 && !memcmp(packet + 24, nonce, 8)) {
          double t2, t3;
          uint8_t stratum;
          if (!lab::ntpReply(packet, got, nonce, t2, t3, stratum)) {
            fail("NTP server unsynchronized or invalid");
            return;
          }
          timeval wall;
          gettimeofday(&wall, nullptr);
          double localReceived = double(wall.tv_sec) * 1000 + wall.tv_usec / 1000.0;
          udp.stop();
          state = Idle;
          clear();
          add("NTP " + server.toString() + " / RTT " + String(now - began) + " ms");
          add("Stratum " + String(stratum));
          time_t epoch = time_t(t3 / 1000);
          tm time;
          localtime_r(&epoch, &time);
          char date[40];
          strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S UTC+3", &time);
          add(date);
          if (localSent >= 1700000000000.0)
            add("Clock offset ms: " + String(((t2 - localSent) + (t3 - localReceived)) / 2.0, 2));
          else
            add("Local clock unset; offset unavailable");
          add("Diagnostic only: device time unchanged");
          report(true);
          return;
        }
      }
      if (now - began >= 3000) fail("NTP timeout: no matching response");
      return;
    }
    if (state == Hash) {
      uint8_t chunk[512];
      for (int part = 0; part < 4; ++part) {
        int got = file.read(chunk, sizeof(chunk));
        if (got < 0 || (got == 0 && file.position() < file.size())) {
          fail("SD read failed before EOF");
          return;
        }
        if (!got) {
          uint8_t digest[32];
          int result = mbedtls_sha256_finish_ret(&hashContext, digest);
          file.close();
          mbedtls_sha256_free(&hashContext);
          hashActive = false;
          state = Idle;
          clear();
          if (result) {
            add("SHA256 finalization failed");
            report(false);
          } else {
            add("SHA256 file:");
            add(lab::hex(digest, 32));
            report(true);
          }
          return;
        }
        if (mbedtls_sha256_update_ret(&hashContext, chunk, got)) {
          fail("SHA256 update failed");
          return;
        }
      }
      if (now - lastPaint >= 500) {
        lastPaint = now;
        clear();
        add("Bytes hashed " + String(file.position()));
        add("Total bytes " + String(file.size()));
        draw();
      }
      return;
    }
  }
} runner;
class ToolPage : public Screen {
  uint8_t id;

 public:
  explicit ToolPage(uint8_t value) : id(value) {}
  const char* title() const override { return runner.title(id); }
  void onEnter() override { runner.enter(id); }
  void onExit() override { runner.leave(); }
  void update(uint32_t now) override { runner.update(now); }
  bool onInput(const InputEvent& e) override { return runner.inputEvent(e); }
};
}  // namespace
Screen* advancedTool(uint8_t index) {
  static ToolPage pages[] = {ToolPage(0),  ToolPage(1),  ToolPage(2),  ToolPage(3),  ToolPage(4),
                             ToolPage(5),  ToolPage(6),  ToolPage(7),  ToolPage(8),  ToolPage(9),
                             ToolPage(10), ToolPage(11), ToolPage(12), ToolPage(13), ToolPage(14),
                             ToolPage(15), ToolPage(16), ToolPage(17), ToolPage(18), ToolPage(19),
                             ToolPage(20), ToolPage(21), ToolPage(22), ToolPage(23), ToolPage(24),
                             ToolPage(25)};
  return index < 26 ? &pages[index] : nullptr;
}
Screen* advancedLab() {
  static const GridItem connection[] = {
      {"جودة WiFi", icon::WIFI, advancedTool(0)},  {"اقتراح قناة", icon::GRAPH, advancedTool(1)},
      {"زمن TCP", icon::TIMER, advancedTool(2)},   {"توفر HTTP", icon::EYE, advancedTool(3)},
      {"تعريف خدمة", icon::INFO, advancedTool(4)}, {"اختبار NTP", icon::TIMER, advancedTool(5)}};
  static const GridItem dns[] = {
      {"AAAA", icon::GRAPH, advancedTool(6)},        {"MX", icon::GRAPH, advancedTool(7)},
      {"TXT", icon::GRAPH, advancedTool(8)},         {"NS", icon::GRAPH, advancedTool(9)},
      {"بحث عكسي", icon::GRAPH, advancedTool(10)},   {"مقارنة DNS", icon::GRAPH, advancedTool(11)},
      {"استقرار DNS", icon::GRAPH, advancedTool(12)}};
  static const GridItem addresses[] = {{"حاسبة شبكة", icon::APPS, advancedTool(13)},
                                       {"تصنيف IPv4", icon::INFO, advancedTool(14)},
                                       {"تداخل CIDR", icon::GRAPH, advancedTool(15)},
                                       {"تنسيق IPv6", icon::APPS, advancedTool(16)},
                                       {"تحليل MAC", icon::INFO, advancedTool(17)}};
  static const GridItem utilities[] = {
      {"كلمة مرور", icon::LOCK, advancedTool(18)}, {"Base64", icon::APPS, advancedTool(19)},
      {"Hex", icon::APPS, advancedTool(20)},       {"ترميز URL", icon::APPS, advancedTool(21)},
      {"بصمة نص", icon::SHIELD, advancedTool(22)}, {"بصمة ملف", icon::SHIELD, advancedTool(23)},
      {"UUID", icon::APPS, advancedTool(24)},      {"زمن النقل", icon::TIMER, advancedTool(25)}};
  static Grid a("فحص الاتصال", connection, 6), b("DNS متقدم", dns, 7),
      c("حساب الشبكات", addresses, 5), d("تحويل وتحقق", utilities, 8);
  static const GridItem categories[] = {{"فحص اتصال", icon::WIFI, &a},
                                        {"DNS متقدم", icon::GRAPH, &b},
                                        {"حساب شبكات", icon::APPS, &c},
                                        {"تحويل وتحقق", icon::SHIELD, &d}};
  static Grid root("المختبر المتقدم", categories, 4);
  return &root;
}
