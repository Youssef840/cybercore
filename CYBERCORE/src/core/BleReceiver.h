#pragma once
#include <BLEDevice.h>
#include <BLEScan.h>
namespace bleRx {
void begin();
void end();
bool start(unsigned seconds);
bool ready();
BLEScanResults results();
void clear();
}  // namespace bleRx
