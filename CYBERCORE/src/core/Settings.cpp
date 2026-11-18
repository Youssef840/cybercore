#include "core/Settings.h"
#include <Preferences.h>

Settings settings;

namespace {
constexpr const char* NS = "cybercore";
constexpr uint8_t SCHEMA = 3;  // bump to reset stale theme/brightness to defaults
}  // namespace

void Settings::load() {
  Preferences p;
  if (!p.begin(NS, true)) return;
  if (p.getUChar("schema", 0) == SCHEMA) {
    brightness = p.getUChar("bright", brightness);
    theme = p.getUChar("theme", theme);
  }
  colorMode = p.getUChar("color", 0) & 7;
  scanMs = constrain(p.getUShort("scanms", 300), 100, 1000);
  channel = min<uint8_t>(p.getUChar("channel", 0), 13);
  minRssi = constrain(p.getChar("rssi", -100), -110, -30);
  alertFrames = constrain(p.getUChar("frames", 5), 1, 50);
  bleSeconds = constrain(p.getUChar("blesec", 3), 1, 10);
  refreshSeconds = constrain(p.getUShort("refresh", 10), 3, 120);
  autoRefresh = p.getBool("auto", false);
  autoSave = p.getBool("autosave", false);
  touchCal = p.getBool("tcal", false);
  touchSwap = p.getBool("tswap", false);
  touchX0 = p.getShort("tx0", 0);
  touchX1 = p.getShort("tx1", 4095);
  touchY0 = p.getShort("ty0", 0);
  touchY1 = p.getShort("ty1", 4095);
  if (abs(touchX1 - touchX0) < 500 || abs(touchY1 - touchY0) < 500) touchCal = false;
  alarmHour = min<uint8_t>(p.getUChar("ah", 7), 23);
  alarmMinute = min<uint8_t>(p.getUChar("am", 0), 59);
  alarmEnabled = p.getBool("ae", false);
  p.end();
  if (brightness < 12) brightness = 12;
  if (theme > 3) theme = 0;
}

void Settings::save() {
  Preferences p;
  if (!p.begin(NS, false)) return;
  p.putUChar("schema", SCHEMA);
  p.putUChar("bright", brightness);
  p.putUChar("theme", theme);
  p.putUChar("color", colorMode);
  p.putUShort("scanms", scanMs);
  p.putUChar("channel", channel);
  p.putChar("rssi", minRssi);
  p.putUChar("frames", alertFrames);
  p.putUChar("blesec", bleSeconds);
  p.putUShort("refresh", refreshSeconds);
  p.putBool("auto", autoRefresh);
  p.putBool("autosave", autoSave);
  p.putBool("tcal", touchCal);
  p.putBool("tswap", touchSwap);
  p.putShort("tx0", touchX0);
  p.putShort("tx1", touchX1);
  p.putShort("ty0", touchY0);
  p.putShort("ty1", touchY1);
  p.putUChar("ah", alarmHour);
  p.putUChar("am", alarmMinute);
  p.putBool("ae", alarmEnabled);
  p.end();
}

void Settings::reset() {
  *this = Settings();
  save();
}
