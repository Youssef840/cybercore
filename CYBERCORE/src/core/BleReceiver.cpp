#include "core/BleReceiver.h"
#include <atomic>
namespace bleRx {
namespace {
std::atomic<bool> complete(false);
bool running = false;
void done(BLEScanResults) { complete.store(true); }
}  // namespace
void begin() {
  BLEDevice::init("");
  complete = false;
  running = false;
  auto s = BLEDevice::getScan();
  s->setActiveScan(false);
  s->setInterval(100);
  s->setWindow(80);
}
void end() {
  if (running) BLEDevice::getScan()->stop();
  running = false;
  complete = false;
  BLEDevice::getScan()->clearResults();
  BLEDevice::deinit(false);
}
bool start(unsigned seconds) {
  complete = false;
  running = BLEDevice::getScan()->start(seconds, done, false);
  return running;
}
bool ready() {
  if (!complete.load()) return false;
  running = false;
  return true;
}
BLEScanResults results() { return BLEDevice::getScan()->getResults(); }
void clear() {
  BLEDevice::getScan()->clearResults();
  complete = false;
}
}  // namespace bleRx
