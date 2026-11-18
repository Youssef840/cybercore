#pragma once
#include <Arduino.h>
namespace lab {
struct Subnet {
  uint32_t ip, network, mask, broadcast;
  uint8_t prefix;
};
bool ipv4(const String& text, uint32_t& out);
String ipText(uint32_t ip);
bool subnet(const String& text, Subnet& out);
String hex(const uint8_t* p, size_t n);
struct DnsRecords {
  uint8_t count = 0;
  char value[8][128] = {};
};
int dnsRecords(const uint8_t* p, size_t n, const uint8_t* q, size_t qn, uint16_t type,
               DnsRecords& out);
bool ntpReply(const uint8_t* p, size_t n, const uint8_t* nonce, double& t2, double& t3,
              uint8_t& stratum);
bool calculate(uint8_t id, const String input[3], String output[40], int& count);
unsigned selfTest();
}  // namespace lab
