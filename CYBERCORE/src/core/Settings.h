#pragma once
// ============================================================================
//  Settings - persistent configuration in NVS (Preferences)
// ============================================================================
#include <Arduino.h>

struct Settings {
  uint8_t brightness = 220;  // backlight 12..255
  uint8_t theme = 0;         // 0 green, 1 amber, 2 cyan, 3 red

  uint8_t colorMode = 0;  // bit0 BGR, bit1 software byte swap, bit2 inversion
  uint16_t scanMs = 300;  // passive Wi-Fi dwell per channel
  uint8_t channel = 0;    // 0 = channel hopping / all channels
  int8_t minRssi = -100;
  uint8_t bleSeconds = 3;
  uint8_t alertFrames = 5;
  uint16_t refreshSeconds = 10;
  bool autoRefresh = false, autoSave = false;
  bool touchCal = false;
  int16_t touchX0 = 0, touchX1 = 4095, touchY0 = 0, touchY1 = 4095;
  bool touchSwap = false;
  uint8_t alarmHour = 7, alarmMinute = 0;
  bool alarmEnabled = false;

  void load();
  void save();
  void reset();
};

extern Settings settings;
