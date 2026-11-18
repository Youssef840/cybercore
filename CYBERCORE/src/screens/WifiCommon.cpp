#include <WiFi.h>
#include "screens/Modules.h"
#include "core/Settings.h"
#include "core/Storage.h"

int wifiScanInto(ApInfo* out, int cap) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(60);
  WiFi.scanNetworks(false, true, true, settings.scanMs, settings.channel);
  return wifiCollect(out, cap);
}

int wifiCollect(ApInfo* out, int cap) {
  int count = WiFi.scanComplete(), n = 0;
  for (int j = 0; j < count && n < cap; ++j) {
    if (WiFi.RSSI(j) < settings.minRssi) continue;
    int i = j;
    ApInfo& a = out[n++];
    strncpy(a.ssid, WiFi.SSID(i).c_str(), 32);
    a.ssid[32] = 0;
    a.hidden = a.ssid[0] == 0;
    if (a.hidden) strcpy(a.ssid, "<مخفية>");
    a.rssi = WiFi.RSSI(i);
    a.channel = WiFi.channel(i);
    a.enc = WiFi.encryptionType(i);
    memset(a.bssid, 0, 6);
    if (WiFi.BSSID(i)) memcpy(a.bssid, WiFi.BSSID(i), 6);
  }
  WiFi.scanDelete();
  // sort by RSSI descending (insertion sort, n is small)
  for (int i = 1; i < n; i++) {
    ApInfo key = out[i];
    int j = i - 1;
    while (j >= 0 && out[j].rssi < key.rssi) {
      out[j + 1] = out[j];
      j--;
    }
    out[j + 1] = key;
  }
  return n;
}

const char* encNameAr(uint8_t enc) {
  switch (enc) {
    case WIFI_AUTH_OPEN:
      return "مفتوحة";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "مؤسسي";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2/3";
    default:
      return "اخرى";
  }
}

bool saveWifiResults(const ApInfo* out, int n, const char* kind) {
  if (n <= 0) return false;
  bool ok = true;
  for (int i = 0; i < n; ++i) {
    const auto& a = out[i];
    char b[18];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", a.bssid[0], a.bssid[1], a.bssid[2],
             a.bssid[3], a.bssid[4], a.bssid[5]);
    String line = String(millis()) + "," + storage::quote(a.ssid) + "," + b + "," +
                  String(a.channel) + "," + String(a.rssi) + "," + String(a.enc) + "," +
                  String(a.hidden);
    if (!storage::append(kind, line, "uptime_ms,ssid,bssid,channel,rssi,auth,hidden")) {
      ok = false;
      break;
    }
  }
  return ok;
}
