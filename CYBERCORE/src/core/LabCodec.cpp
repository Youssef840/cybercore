#include "core/LabCodec.h"
#include "core/Protocol.h"
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_system.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha256.h>
#include <math.h>
namespace lab {
namespace {
uint16_t be16(const uint8_t* p) { return (uint16_t(p[0]) << 8) | p[1]; }
uint32_t be32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
int nibble(char c) {
  return c >= '0' && c <= '9'   ? c - '0'
         : c >= 'a' && c <= 'f' ? c - 'a' + 10
         : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                : -1;
}
bool number(const String& s, unsigned long& v) {
  if (!s.length()) return false;
  v = 0;
  for (unsigned i = 0; i < s.length(); ++i) {
    if (s[i] < '0' || s[i] > '9' || v > 429496729UL || (v == 429496729UL && s[i] > '5'))
      return false;
    v = v * 10 + s[i] - '0';
  }
  return true;
}
String printable(const uint8_t* p, size_t n) {
  String out;
  char b[5];
  for (size_t i = 0; i < n; ++i) {
    if (p[i] >= 32 && p[i] < 127)
      out += char(p[i]);
    else {
      snprintf(b, sizeof(b), "\\x%02X", p[i]);
      out += b;
    }
  }
  return out;
}
bool name(const uint8_t* p, size_t n, size_t& pos, String& out) {
  size_t cursor = pos;
  bool jumped = false;
  out = "";
  for (int hops = 0; hops < 128 && cursor < n; ++hops) {
    size_t start = cursor;
    uint8_t len = p[cursor++];
    if (!len) {
      if (!jumped) pos = cursor;
      return true;
    }
    if ((len & 0xc0) == 0xc0) {
      if (cursor >= n) return false;
      size_t target = ((len & 0x3f) << 8) | p[cursor++];
      if (target < 12 || target >= start) return false;
      if (!jumped) pos = cursor;
      jumped = true;
      cursor = target;
      continue;
    }
    if ((len & 0xc0) || len > n - cursor || out.length() + len + 1 > 253) return false;
    if (out.length()) out += '.';
    for (unsigned i = 0; i < len; ++i) {
      uint8_t c = p[cursor++];
      out += (c >= 33 && c < 127) ? char(c) : '?';
    }
  }
  return false;
}
uint32_t bounded(uint32_t size) {
  uint32_t limit = UINT32_MAX - (UINT32_MAX % size), value;
  do {
    value = esp_random();
  } while (value >= limit);
  return value % size;
}
bool entropyReady() {
  return WiFi.getMode() != WIFI_MODE_NULL ||
         esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED;
}
}  // namespace
bool ipv4(const String& text, uint32_t& out) {
  out = 0;
  int octets = 0;
  unsigned value = 0, digits = 0;
  for (unsigned i = 0; i <= text.length(); ++i) {
    char c = i < text.length() ? text[i] : '.';
    if (c == '.') {
      if (!digits || ++octets > 4) return false;
      out = (out << 8) | value;
      value = digits = 0;
    } else if (c >= '0' && c <= '9') {
      if (++digits > 3) return false;
      value = value * 10 + c - '0';
      if (value > 255) return false;
    } else
      return false;
  }
  return octets == 4;
}
String ipText(uint32_t ip) {
  char b[16];
  snprintf(b, sizeof(b), "%u.%u.%u.%u", uint8_t(ip >> 24), uint8_t(ip >> 16), uint8_t(ip >> 8),
           uint8_t(ip));
  return b;
}
bool subnet(const String& text, Subnet& out) {
  int slash = text.indexOf('/');
  unsigned long prefix;
  if (slash < 1 || !ipv4(text.substring(0, slash), out.ip) ||
      !number(text.substring(slash + 1), prefix) || prefix > 32)
    return false;
  out.prefix = prefix;
  out.mask = prefix ? UINT32_MAX << (32 - prefix) : 0;
  out.network = out.ip & out.mask;
  out.broadcast = out.network | ~out.mask;
  return true;
}
String hex(const uint8_t* p, size_t n) {
  String result;
  result.reserve(n * 2);
  static const char* d = "0123456789abcdef";
  for (size_t i = 0; i < n; ++i) {
    result += d[p[i] >> 4];
    result += d[p[i] & 15];
  }
  return result;
}
int dnsRecords(const uint8_t* p, size_t n, const uint8_t* q, size_t qn, uint16_t type,
               DnsRecords& out) {
  out = DnsRecords();
  if (n < 12 || qn < 17 || n < qn || p[0] != q[0] || p[1] != q[1] || !(p[2] & 0x80) ||
      (p[2] & 0x78) || be16(p + 4) != 1 || memcmp(p + 12, q + 12, qn - 12))
    return 1;
  if (p[2] & 2) return 2;
  if (p[3] & 15) return 16 + (p[3] & 15);
  size_t pos = qn;
  unsigned total = be16(p + 6);
  if (total > 64) return 1;
  for (unsigned i = 0; i < total; ++i) {
    String owner;
    if (!name(p, n, pos, owner) || n - pos < 10) return 1;
    unsigned rt = be16(p + pos), cls = be16(p + pos + 2), length = be16(p + pos + 8);
    pos += 10;
    if (length > n - pos) return 1;
    size_t end = pos + length;
    if (rt == type && cls == 1) {
      String value;
      if (type == 1) {
        if (length != 4) return 1;
        value = ipText(be32(p + pos));
      } else if (type == 28) {
        if (length != 16) return 1;
        char b[48];
        in6_addr addr;
        memcpy(&addr, p + pos, 16);
        if (!inet_ntop(AF_INET6, &addr, b, sizeof(b))) return 1;
        value = b;
        value.toLowerCase();
      } else if (type == 2 || type == 12 || type == 15) {
        size_t cursor = pos;
        if (type == 15) {
          if (length < 3) return 1;
          value = String(be16(p + cursor)) + " ";
          cursor += 2;
        }
        String domain;
        if (!name(p, n, cursor, domain) || cursor != end) return 1;
        value += domain;
      } else if (type == 16) {
        size_t cursor = pos;
        while (cursor < end) {
          uint8_t len = p[cursor++];
          if (len > end - cursor) return 1;
          value += printable(p + cursor, len);
          cursor += len;
        }
        if (value.length() > 127) value = value.substring(0, 124) + "...";
      } else
        return 1;
      if (out.count < 8) strlcpy(out.value[out.count++], value.c_str(), 128);
    }
    pos = end;
  }
  return out.count ? 0 : 3;
}
bool ntpReply(const uint8_t* p, size_t n, const uint8_t* nonce, double& t2, double& t3,
              uint8_t& stratum) {
  if (n < 48 || (p[0] & 7) != 4 || ((p[0] >> 3) & 7) < 3 || ((p[0] >> 3) & 7) > 4 ||
      (p[0] >> 6) == 3 || p[1] < 1 || p[1] > 15 || memcmp(p + 24, nonce, 8))
    return false;
  auto timestamp = [&](const uint8_t* stamp) {
    uint32_t sec = be32(stamp);
    double unixSeconds =
        sec >= 2208988800UL ? double(sec) - 2208988800.0 : double(sec) + 2085978496.0;
    return unixSeconds * 1000.0 + double(be32(stamp + 4)) * 1000.0 / 4294967296.0;
  };
  if (!be32(p + 40) || !be32(p + 32)) return false;
  t2 = timestamp(p + 32);
  t3 = timestamp(p + 40);
  if (t3 < t2 || t3 - t2 > 10000) return false;
  stratum = p[1];
  return true;
}
bool calculate(uint8_t id, const String input[3], String output[40], int& count) {
  count = 0;
  auto add = [&](String s) {
    if (!s.length() && count < 40) output[count++] = "(empty)";
    while (s.length() && count < 40) {
      output[count++] = s.substring(0, 44);
      s.remove(0, min<unsigned>(44, s.length()));
    }
  };
  auto fail = [&](const char* why) {
    add(why);
    return false;
  };
  if (input[0].length() > 63 || input[1].length() > 63 || input[2].length() > 63)
    return fail("Input exceeds 63 bytes");
  if (id == 13) {
    Subnet s;
    if (!subnet(input[0], s)) return fail("Invalid IPv4/CIDR");
    add("Network: " + ipText(s.network));
    add("Mask: " + ipText(s.mask));
    add("Last: " + ipText(s.broadcast));
    uint64_t addresses = uint64_t(1) << (32 - s.prefix),
             hosts = s.prefix >= 31 ? addresses : addresses - 2;
    char b[48];
    snprintf(b, sizeof(b), "Addresses %llu / hosts %llu", addresses, hosts);
    add(b);
    add("First host: " + ipText(s.prefix >= 31 ? s.network : s.network + 1));
    add("Last host: " + ipText(s.prefix >= 31 ? s.broadcast : s.broadcast - 1));
    return true;
  }
  if (id == 14) {
    uint32_t ip;
    if (!ipv4(input[0], ip)) return fail("Invalid IPv4");
    add(ipText(ip));
    String kind = "Other / check special-use registry";
    if (ip == 0)
      kind = "Unspecified";
    else if ((ip >> 24) == 127)
      kind = "Loopback";
    else if ((ip >> 24) == 10 || (ip >> 20) == 0xac1 || (ip >> 16) == 0xc0a8)
      kind = "Private RFC1918";
    else if ((ip >> 16) == 0xa9fe)
      kind = "Link-local";
    else if ((ip >> 22) == 0x191)
      kind = "Shared CGNAT";
    else if ((ip >> 28) == 14)
      kind = "Multicast";
    else if (ip == UINT32_MAX)
      kind = "Limited broadcast";
    else if ((ip >> 28) == 15)
      kind = "Reserved";
    else if ((ip >> 24) == 0)
      kind = "This-network range";
    else if ((ip >> 24) == 192 && ((ip >> 8) == 0xc00002))
      kind = "Documentation";
    else if ((ip >> 8) == 0xc63364 || (ip >> 8) == 0xcb0071)
      kind = "Documentation";
    else if ((ip >> 17) == 0x6309)
      kind = "Benchmark network";
    add(kind);
    char b[40];
    snprintf(b, sizeof(b), "Integer %lu / hex %08lX", (unsigned long)ip, (unsigned long)ip);
    add(b);
    return true;
  }
  if (id == 15) {
    Subnet a, b;
    if (!subnet(input[0], a) || !subnet(input[1], b)) return fail("Invalid IPv4/CIDR pair");
    bool overlap = max(a.network, b.network) <= min(a.broadcast, b.broadcast);
    add(overlap ? "OVERLAP: YES" : "OVERLAP: NO");
    if (overlap) {
      add("First: " + ipText(max(a.network, b.network)));
      add("Last: " + ipText(min(a.broadcast, b.broadcast)));
    }
    add(a.network <= b.network && a.broadcast >= b.broadcast ? "A contains B"
                                                             : "A does not contain B");
    add(b.network <= a.network && b.broadcast >= a.broadcast ? "B contains A"
                                                             : "B does not contain A");
    return true;
  }
  if (id == 16) {
    in6_addr addr;
    if (inet_pton(AF_INET6, input[0].c_str(), &addr) != 1) return fail("Invalid IPv6");
    char b[64];
    inet_ntop(AF_INET6, &addr, b, sizeof(b));
    add("Compressed:");
    String normalized = b;
    normalized.toLowerCase();
    add(normalized);
    uint8_t* p = reinterpret_cast<uint8_t*>(&addr);
    snprintf(b, sizeof(b),
             "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x", p[0], p[1],
             p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14],
             p[15]);
    add("Expanded:");
    add(b);
    return true;
  }
  if (id == 17) {
    String value = input[0];
    value.replace(":", "");
    value.replace("-", "");
    if (value.length() != 12) return fail("MAC needs six octets");
    uint8_t bytes[6];
    for (int i = 0; i < 6; ++i) {
      int a = nibble(value[2 * i]), b = nibble(value[2 * i + 1]);
      if (a < 0 || b < 0) return fail("Invalid MAC hex");
      bytes[i] = (a << 4) | b;
    }
    add("MAC: " + hex(bytes, 6));
    add(bytes[0] & 1 ? "Multicast/group" : "Unicast/individual");
    add(bytes[0] & 2 ? "Locally administered" : "Universally administered flag");
    bool all = true;
    for (auto b : bytes)
      if (b != 255) all = false;
    if (all) add("Broadcast MAC");
    add("Flags do not identify device/vendor");
    return true;
  }
  if (id == 18) {
    unsigned long length;
    if (!number(input[0], length) || length < 12 || length > 48)
      return fail("Length must be 12..48");
    if (!entropyReady()) return fail("Enable WiFi/BLE radio for RNG entropy");
    const char* groups[] = {"ABCDEFGHJKLMNPQRSTUVWXYZ", "abcdefghijkmnopqrstuvwxyz", "23456789",
                            "!@#$%&*+-_="};
    String alphabet = String(groups[0]) + groups[1] + groups[2] + groups[3], value;
    for (int i = 0; i < 4; ++i) value += groups[i][bounded(strlen(groups[i]))];
    while (value.length() < length) value += alphabet[bounded(alphabet.length())];
    for (int i = length - 1; i > 0; --i) {
      unsigned j = bounded(i + 1);
      char c = value[i];
      value.setCharAt(i, value[j]);
      value.setCharAt(j, c);
    }
    add(value);
    add("Displayed only; SD export disabled");
    return true;
  }
  if (id == 19) {
    uint8_t data[256];
    size_t size = 0;
    int result;
    if (input[1] == "decode")
      result = mbedtls_base64_decode(data, sizeof(data), &size,
                                     reinterpret_cast<const uint8_t*>(input[0].c_str()),
                                     input[0].length());
    else if (input[1] == "encode")
      result = mbedtls_base64_encode(data, sizeof(data), &size,
                                     reinterpret_cast<const uint8_t*>(input[0].c_str()),
                                     input[0].length());
    else
      return fail("Mode: encode or decode");
    if (result) return fail("Invalid Base64 or size");
    add(printable(data, size));
    return true;
  }
  if (id == 20) {
    if (input[1] == "encode")
      add(hex(reinterpret_cast<const uint8_t*>(input[0].c_str()), input[0].length()));
    else if (input[1] == "decode") {
      String value = input[0];
      value.replace(" ", "");
      if (value.length() % 2) return fail("Odd number of hex digits");
      uint8_t data[64];
      if (value.length() > 128) return fail("Hex input too long");
      for (unsigned i = 0; i < value.length() / 2; ++i) {
        int a = nibble(value[2 * i]), b = nibble(value[2 * i + 1]);
        if (a < 0 || b < 0) return fail("Invalid hex");
        data[i] = (a << 4) | b;
      }
      add(printable(data, value.length() / 2));
    } else
      return fail("Mode: encode or decode");
    return true;
  }
  if (id == 21) {
    String result;
    static const char* d = "0123456789ABCDEF";
    if (input[1] == "encode") {
      for (unsigned i = 0; i < input[0].length(); ++i) {
        uint8_t c = input[0][i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            strchr("-._~", c))
          result += char(c);
        else {
          result += '%';
          result += d[c >> 4];
          result += d[c & 15];
        }
      }
    } else if (input[1] == "decode") {
      uint8_t data[64];
      size_t n = 0;
      for (unsigned i = 0; i < input[0].length(); ++i) {
        uint8_t c = input[0][i];
        if (c == '%') {
          if (i + 2 >= input[0].length()) return fail("Incomplete percent sequence");
          int a = nibble(input[0][i + 1]), b = nibble(input[0][i + 2]);
          if (a < 0 || b < 0) return fail("Invalid percent sequence");
          c = (a << 4) | b;
          i += 2;
        }
        data[n++] = c;
      }
      result = printable(data, n);
    } else
      return fail("Mode: encode or decode");
    add(result);
    return true;
  }
  if (id == 22) {
    uint8_t digest[32];
    if (mbedtls_sha256_ret(reinterpret_cast<const uint8_t*>(input[0].c_str()), input[0].length(),
                           digest, 0))
      return fail("SHA256 failed");
    add(hex(digest, 32));
    return true;
  }
  if (id == 24) {
    uint8_t bytes[16];
    esp_fill_random(bytes, sizeof(bytes));
    bytes[6] = (bytes[6] & 15) | 0x40;
    bytes[8] = (bytes[8] & 63) | 0x80;
    String value = hex(bytes, 16);
    add(value.substring(0, 8) + "-" + value.substring(8, 12) + "-" + value.substring(12, 16) + "-" +
        value.substring(16, 20) + "-" + value.substring(20));
    add("UUID v4; not an authentication token");
    return true;
  }
  if (id == 25) {
    char* end;
    double size = strtod(input[0].c_str(), &end);
    if (!input[0].length() || *end || !isfinite(size) || size <= 0 || size > 1e9)
      return fail("MiB must be >0 and <=1e9");
    double speed = strtod(input[1].c_str(), &end);
    if (!input[1].length() || *end || !isfinite(speed) || speed < 0.001 || speed > 1e6)
      return fail("Mbps must be .001..1e6");
    double seconds = size * 8388608.0 / (speed * 1e6);
    char b[64];
    snprintf(b, sizeof(b), "Ideal seconds: %.3f", seconds);
    add(b);
    snprintf(b, sizeof(b), "%.3f MiB / %.3f Mbps", size, speed);
    add(b);
    add("Excludes overhead and retransmissions");
    return true;
  }
  return fail("Unsupported calculator");
}
unsigned selfTest() {
  unsigned failed = 0;
  String in[3], out[40];
  int count;
  uint32_t ip;
  Subnet s;
  if (!ipv4("192.168.1.1", ip) || ip != 0xc0a80101 || ipv4("1.2.3.256", ip) || ipv4("1.2.3.", ip))
    ++failed;
  if (!subnet("10.1.2.3/16", s) || s.network != 0x0a010000 || s.broadcast != 0x0a01ffff ||
      subnet("1.2.3.4/33", s))
    ++failed;
  in[0] = "192.168.1.1/24";
  if (!calculate(13, in, out, count) || out[0] != "Network: 192.168.1.0") ++failed;
  in[0] = "127.0.0.1";
  if (!calculate(14, in, out, count) || out[1] != "Loopback") ++failed;
  in[0] = "192.168.1.0/24";
  in[1] = "192.168.1.128/25";
  if (!calculate(15, in, out, count) || out[0] != "OVERLAP: YES") ++failed;
  in[0] = "2001:0db8:0:0:0:0:0:1";
  if (!calculate(16, in, out, count) || out[1] != "2001:db8::1") ++failed;
  in[0] = "02:00:00:00:00:01";
  if (!calculate(17, in, out, count) || out[2] != "Locally administered") ++failed;
  in[0] = "abc";
  in[1] = "encode";
  if (!calculate(19, in, out, count) || out[0] != "YWJj") ++failed;
  in[0] = "YWJj";
  in[1] = "decode";
  if (!calculate(19, in, out, count) || out[0] != "abc") ++failed;
  in[0] = "414200";
  if (!calculate(20, in, out, count) || out[0] != "AB\\x00") ++failed;
  in[0] = "a b+";
  in[1] = "encode";
  if (!calculate(21, in, out, count) || out[0] != "a%20b%2B") ++failed;
  in[0] = "%GG";
  in[1] = "decode";
  if (calculate(21, in, out, count)) ++failed;
  in[0] = "abc";
  if (!calculate(22, in, out, count) ||
      out[0] + out[1] != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
    ++failed;
  if (!calculate(24, in, out, count) || out[0].length() != 36 || out[0][14] != '4' ||
      String("89ab").indexOf(out[0][19]) < 0)
    ++failed;
  in[0] = "1";
  in[1] = "8.388608";
  if (!calculate(25, in, out, count) || out[0] != "Ideal seconds: 1.000") ++failed;
  uint8_t q[80], p[128];
  size_t qn = protocol::dnsQuery("example.com", 0x1234, q, sizeof(q));
  q[qn - 3] = 28;
  memcpy(p, q, qn);
  p[2] = 0x81;
  p[3] = 0x80;
  p[7] = 1;
  const uint8_t answer[] = {192,  12,   0, 28, 0, 1, 0, 0, 0, 60, 0, 16, 0x20, 1,
                            0x0d, 0xb8, 0, 0,  0, 0, 0, 0, 0, 0,  0, 0,  0,    1};
  memcpy(p + qn, answer, sizeof(answer));
  DnsRecords records;
  if (dnsRecords(p, qn + sizeof(answer), q, qn, 28, records) ||
      strcmp(records.value[0], "2001:db8::1"))
    ++failed;
  if (dnsRecords(p, qn + sizeof(answer) - 1, q, qn, 28, records) != 1) ++failed;
  auto response = [&](uint16_t type, const uint8_t* data, size_t len) {
    q[qn - 4] = type >> 8;
    q[qn - 3] = type;
    memcpy(p, q, qn);
    p[2] = 0x81;
    p[3] = 0x80;
    p[7] = 1;
    uint8_t head[] = {192, 12, uint8_t(type >> 8), uint8_t(type), 0, 1, 0, 0, 0,
                      60,  0,  uint8_t(len)};
    memcpy(p + qn, head, 12);
    memcpy(p + qn + 12, data, len);
    return dnsRecords(p, qn + 12 + len, q, qn, type, records);
  };
  const uint8_t mx[] = {0, 10, 4, 'm', 'a', 'i', 'l', 192, 12};
  if (response(15, mx, sizeof(mx)) || strcmp(records.value[0], "10 mail.example.com")) ++failed;
  const uint8_t txt[] = {2, 'a', 'b', 2, 'c', 'd'};
  if (response(16, txt, sizeof(txt)) || strcmp(records.value[0], "abcd")) ++failed;
  const uint8_t invalidTxt[] = {3, 'a'};
  if (response(16, invalidTxt, sizeof(invalidTxt)) != 1) ++failed;
  const uint8_t compressed[] = {192, 12};
  if (response(2, compressed, 2) || strcmp(records.value[0], "example.com")) ++failed;
  if (response(12, compressed, 2) || strcmp(records.value[0], "example.com")) ++failed;
  const uint8_t selfPointer[] = {192, uint8_t(qn + 12)};
  if (response(2, selfPointer, 2) != 1) ++failed;
  in[0] = "0.0.0.0/0";
  if (!calculate(13, in, out, count) || out[2] != "Last: 255.255.255.255") ++failed;
  in[0] = "192.168.1.0/31";
  if (!calculate(13, in, out, count) || out[3] != "Addresses 2 / hosts 2") ++failed;
  in[0] = "1.2.3.4/32";
  if (!calculate(13, in, out, count) || out[3] != "Addresses 1 / hosts 1") ++failed;
  uint8_t ntp[48] = {}, nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  ntp[0] = 0x24;
  ntp[1] = 2;
  memcpy(ntp + 24, nonce, 8);
  ntp[32] = ntp[40] = 0xe8;
  ntp[33] = ntp[41] = 0x00;
  double t2, t3;
  uint8_t stratum;
  if (!ntpReply(ntp, 48, nonce, t2, t3, stratum) || stratum != 2) ++failed;
  ntp[0] |= 0xc0;
  if (ntpReply(ntp, 48, nonce, t2, t3, stratum)) ++failed;
  return failed;
}
}  // namespace lab
