#include "core/Protocol.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
namespace protocol {
namespace {
uint16_t le(const uint8_t* p) { return p[0] | (uint16_t(p[1]) << 8); }
uint16_t be(const uint8_t* p) { return (uint16_t(p[0]) << 8) | p[1]; }
uint8_t pmf(const uint8_t* p, size_t n) {
  if (n < 8 || le(p) != 1) return 0;
  size_t pos = 6;
  uint16_t pairs = le(p + pos);
  pos += 2;
  if (!pairs || pairs > 16 || pairs > (n - pos) / 4) return 0;
  pos += pairs * 4;
  if (n - pos < 2) return 0;
  uint16_t akms = le(p + pos);
  pos += 2;
  if (!akms || akms > 16 || akms > (n - pos) / 4) return 0;
  pos += akms * 4;
  if (pos == n) return 1;
  if (n - pos < 2) return 0;
  uint16_t caps = le(p + pos);
  return caps & 0x80 ? (caps & 0x40 ? 3 : 2) : 1;
}
bool skipName(const uint8_t* p, size_t n, size_t& pos) {
  for (unsigned labels = 0; labels < 128 && pos < n; ++labels) {
    size_t start = pos;
    uint8_t len = p[pos++];
    if (!len) return true;
    if ((len & 0xc0) == 0xc0) {
      if (pos >= n) return false;
      size_t target = ((len & 0x3f) << 8) | p[pos++];
      return target >= 12 && target < start;
    }
    if ((len & 0xc0) || len > n - pos) return false;
    pos += len;
  }
  return false;
}
}  // namespace
bool advertisement(const uint8_t* p, size_t n, Advertisement& out) {
  out = Advertisement();
  size_t pos = 0;
  while (pos < n) {
    if (n - pos < 2) return false;
    uint8_t type = p[pos++], len = p[pos++];
    if (len > n - pos) return false;
    if (type == 0) {
      if (len > 32) return false;
      bool visible = false;
      for (unsigned i = 0; i < len; ++i)
        if (p[pos + i]) visible = true;
      if (visible) {
        for (unsigned i = 0; i < len;) {
          uint8_t c = p[pos + i];
          unsigned size = c < 128                    ? 1
                          : (c >= 0xc2 && c <= 0xdf) ? 2
                          : (c >= 0xe0 && c <= 0xef) ? 3
                          : (c >= 0xf0 && c <= 0xf4) ? 4
                                                     : 0;
          bool valid = size && i + size <= len;
          for (unsigned j = 1; valid && j < size; ++j)
            if ((p[pos + i + j] & 0xc0) != 0x80) valid = false;
          if (valid && size == 3 &&
              ((c == 0xe0 && p[pos + i + 1] < 0xa0) || (c == 0xed && p[pos + i + 1] >= 0xa0)))
            valid = false;
          if (valid && size == 4 &&
              ((c == 0xf0 && p[pos + i + 1] < 0x90) || (c == 0xf4 && p[pos + i + 1] >= 0x90)))
            valid = false;
          if (valid && (c >= 32 && c != 127)) {
            memcpy(out.ssid + i, p + pos + i, size);
            i += size;
          } else {
            out.ssid[i++] = '?';
          }
        }
        out.ssid[len] = 0;
      }
    }
    if (type == 48) out.pmf = pmf(p + pos, len);
    if (type == 221 && len >= 4 && p[pos] == 0 && p[pos + 1] == 0x50 && p[pos + 2] == 0xf2 &&
        p[pos + 3] == 4)
      out.wps = true;
    pos += len;
  }
  return true;
}
size_t dnsQuery(const char* host, uint16_t id, uint8_t* out, size_t cap) {
  if (!host || cap < 18) return 0;
  size_t len = strlen(host);
  if (!len || len > 253) return 0;
  if (len + 18 > cap) return 0;
  memset(out, 0, 12);
  out[0] = id >> 8;
  out[1] = id;
  out[2] = 1;
  out[5] = 1;
  size_t pos = 12, start = 0;
  for (size_t i = 0; i <= len; ++i) {
    if (i == len || host[i] == '.') {
      size_t size = i - start;
      if (!size || size > 63 || host[start] == '-' || host[i - 1] == '-') return 0;
      out[pos++] = size;
      memcpy(out + pos, host + start, size);
      pos += size;
      start = i + 1;
    } else {
      char c = host[i];
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-'))
        return 0;
    }
  }
  out[pos++] = 0;
  out[pos++] = 0;
  out[pos++] = 1;
  out[pos++] = 0;
  out[pos++] = 1;
  return pos;
}
int dnsAnswer(const uint8_t* p, size_t n, const uint8_t* q, size_t qn, uint8_t ip[4]) {
  if (n < 12 || qn < 17 || n < qn || p[0] != q[0] || p[1] != q[1] || !(p[2] & 0x80) ||
      (p[2] & 0x78) || be(p + 4) != 1)
    return 1;
  if (p[2] & 2) return 2;
  if (memcmp(p + 12, q + 12, qn - 12)) return 1;
  if (p[3] & 15) return 16 + (p[3] & 15);
  size_t pos = qn;
  unsigned answers = be(p + 6);
  if (answers > 64) return 1;
  bool found = false;
  uint8_t result[4] = {};
  for (unsigned i = 0; i < answers; ++i) {
    if (!skipName(p, n, pos) || n - pos < 10) return 1;
    unsigned type = be(p + pos), klass = be(p + pos + 2), len = be(p + pos + 8);
    pos += 10;
    if (len > n - pos) return 1;
    if (type == 1 && klass == 1) {
      if (len != 4) return 1;
      if (!found) {
        memcpy(result, p + pos, 4);
        found = true;
      }
    }
    pos += len;
  }
  if (found) {
    memcpy(ip, result, 4);
    return 0;
  }
  return 3;
}
bool httpHeader(const char* line, HttpHeaders& out) {
  if (!line) return false;
  size_t n = strlen(line);
  if (n > 255) return false;
  if (!strncmp(line, "HTTP/1.", 7)) {
    if (out.code) return false;
    if (n < 12 || (line[7] != '0' && line[7] != '1') || line[8] != ' ' || line[9] < '1' ||
        line[9] > '5' || line[10] < '0' || line[10] > '9' || line[11] < '0' || line[11] > '9' ||
        (n > 12 && line[12] != ' '))
      return false;
    for (size_t i = 12; i < n; ++i)
      if ((uint8_t(line[i]) < 32 && line[i] != '\t') || uint8_t(line[i]) == 127) return false;
    out = HttpHeaders();
    out.code = (line[9] - '0') * 100 + (line[10] - '0') * 10 + line[11] - '0';
    return true;
  }
  const char* colon = strchr(line, ':');
  if (!colon || colon == line) return false;
  size_t key = colon - line;
  for (size_t i = 0; i < key; ++i) {
    unsigned char c = line[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
          strchr("!#$%&'*+-.^_`|~", c)))
      return false;
  }
  const char* value = colon + 1;
  while (*value == ' ' || *value == '\t') value++;
  for (const char* p = value; *p; ++p)
    if ((uint8_t(*p) < 32 && *p != '\t') || uint8_t(*p) == 127) return false;
  auto match = [&](const char* name) {
    return strlen(name) == key && !strncasecmp(line, name, key);
  };
  auto copy = [&](char* dest, size_t cap) {
    size_t size = strlen(value);
    while (size && (value[size - 1] == ' ' || value[size - 1] == '\t')) size--;
    size = size < cap - 1 ? size : cap - 1;
    memcpy(dest, value, size);
    dest[size] = 0;
  };
  if (match("server")) copy(out.server, sizeof(out.server));
  if (match("location")) copy(out.location, sizeof(out.location));
  if (*value) {
    if (match("content-security-policy")) out.flags |= 1;
    if (match("x-frame-options")) out.flags |= 2;
    if (match("x-content-type-options")) out.flags |= 4;
    if (match("referrer-policy")) out.flags |= 8;
    if (match("strict-transport-security")) out.flags |= 16;
  }
  return true;
}
unsigned selfTest() {
  unsigned failures = 0;
  Advertisement a;
  const uint8_t rsn[] = {0, 1, 'A', 48, 20, 1,   0, 0,   15, 172, 4, 1, 0,  0,   15, 172,
                         4, 1, 0,   0,  15, 172, 2, 192, 0,  221, 4, 0, 80, 242, 4};
  if (!advertisement(rsn, sizeof(rsn), a) || a.pmf != 3 || !a.wps || strcmp(a.ssid, "A"))
    ++failures;
  if (advertisement(rsn, sizeof(rsn) - 1, a)) ++failures;
  const uint8_t broken[] = {48, 8, 1, 0, 0, 15, 172, 4, 255, 255};
  if (!advertisement(broken, sizeof(broken), a) || a.pmf != 0) ++failures;
  uint8_t q[300], p[512], ip[4];
  size_t qn = dnsQuery("example.com", 0x1234, q, sizeof(q));
  if (qn != 29 || dnsQuery("a..com", 1, p, sizeof(p)) || dnsQuery("-a.com", 1, p, sizeof(p)))
    ++failures;
  memcpy(p, q, qn);
  p[2] = 0x81;
  p[3] = 0x80;
  p[7] = 1;
  const uint8_t answer[] = {192, 12, 0, 1, 0, 1, 0, 0, 0, 60, 0, 4, 1, 2, 3, 4};
  memcpy(p + qn, answer, sizeof(answer));
  if (dnsAnswer(p, qn + 16, q, qn, ip) != 0 || ip[0] != 1 || ip[3] != 4) ++failures;
  if (dnsAnswer(p, qn + 15, q, qn, ip) != 1) ++failures;
  p[0] ^= 1;
  if (dnsAnswer(p, qn + 16, q, qn, ip) != 1) ++failures;
  p[0] ^= 1;
  p[2] |= 2;
  if (dnsAnswer(p, qn + 16, q, qn, ip) != 2) ++failures;
  p[2] &= ~2;
  p[3] = 0x83;
  if (dnsAnswer(p, qn + 16, q, qn, ip) != 19) ++failures;
  p[3] = 0x80;
  p[qn] = 192;
  p[qn + 1] = qn;
  if (dnsAnswer(p, qn + 16, q, qn, ip) != 1) ++failures;
  HttpHeaders h;
  if (!httpHeader("HTTP/1.1 200 OK", h) || h.code != 200) ++failures;
  if (!httpHeader("x-CoNtEnT-tYpE-oPtIoNs: nosniff", h) || !(h.flags & 4)) ++failures;
  if (!httpHeader("Server: unit-test  ", h) || strcmp(h.server, "unit-test")) ++failures;
  if (httpHeader("Bad Header: value", h) || httpHeader("HTTP/1.1 2xx OK", h) ||
      httpHeader("X: value\rInjected: x", h))
    ++failures;
  if (httpHeader("HTTP/1.1 404 Not Found", h)) ++failures;
  if (!httpHeader("Content-Security-Policy:", h) || (h.flags & 1)) ++failures;
  char oversized[258];
  memset(oversized, 'A', 257);
  oversized[257] = 0;
  if (httpHeader(oversized, h)) ++failures;
  return failures;
}
}  // namespace protocol
