#pragma once
#include <WiFi.h>
namespace netScope {
inline bool localHost(IPAddress target) {
  if (WiFi.status() != WL_CONNECTED || !uint32_t(WiFi.subnetMask())) return false;
  auto me = WiFi.localIP(), mask = WiFi.subnetMask();
  bool network = true, broadcast = true;
  for (int i = 0; i < 4; ++i) {
    if ((target[i] & mask[i]) != (me[i] & mask[i])) return false;
    if ((target[i] & uint8_t(~mask[i])) != 0) network = false;
    if ((target[i] | mask[i]) != 255) broadcast = false;
  }
  return target[0] > 0 && target[0] < 224 && target != me && !network && !broadcast;
}
}  // namespace netScope
