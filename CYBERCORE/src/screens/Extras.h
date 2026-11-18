#pragma once
#include "screens/Screen.h"
Screen* advancedLab();
Screen* advancedTool(uint8_t index);
Screen* wifiWatch();
Screen* bleInspector();
Screen* httpCheck();
Screen* beaconAudit();
Screen* serviceCheck();
Screen* dnsCheck();
Screen* sdFiles();
Screen* hiddenNetworks();
Screen* bleTracker();
Screen* clockScreen();
Screen* toolOptions();
void trackBle(const char* addr, const char* name);
void clockTick(uint32_t now);
bool dismissAlarm();
