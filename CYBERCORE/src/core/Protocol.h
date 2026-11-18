#pragma once
#include <stddef.h>
#include <stdint.h>
namespace protocol {
// 0 unknown/no RSN; 1 not advertised; 2 capable; 3 required.
struct Advertisement {
  char ssid[33] = {};
  uint8_t pmf = 0;
  bool wps = false;
};
bool advertisement(const uint8_t* ies, size_t n, Advertisement& out);
size_t dnsQuery(const char* host, uint16_t id, uint8_t* out, size_t cap);
// 0 IPv4 answer, 1 malformed/mismatched, 2 truncated, 3 valid response without A;
// 16 + DNS RCODE for an error response.
int dnsAnswer(const uint8_t* packet, size_t n, const uint8_t* query, size_t querySize,
              uint8_t ip[4]);
struct HttpHeaders {
  uint16_t code = 0, flags = 0;
  char server[49] = {}, location[65] = {};
};
bool httpHeader(const char* line, HttpHeaders& out);
unsigned selfTest();
}  // namespace protocol
