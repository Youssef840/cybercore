#include "core/App.h"
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "core/Settings.h"
#include "screens/Modules.h"
#include "screens/Extras.h"
#include "core/Touch.h"
#include "core/Protocol.h"
#include "core/LabCodec.h"
#include "ui/UI.h"

// Provided by MainMenu.cpp
Screen* rootScreen();

App& App::instance() {
  static App app;
  return app;
}

void App::begin() {
  Serial.begin(115200);
  delay(40);
  Serial.printf("\n[%s] v%s booting\n", FW_NAME, FW_VERSION);

  // On-board RGB LED (active LOW) off
  for (uint8_t p : {PIN_LED_R, PIN_LED_G, PIN_LED_B}) {
    if (p == PIN_NONE) continue;
    pinMode(p, OUTPUT);
    digitalWrite(p, HIGH);
  }

  WiFi.mode(WIFI_MODE_NULL);  // radios idle until a module needs them
  settings.load();
  setenv("TZ", "AST-3", 1);
  tzset();
  Theme::applyTheme(settings.theme);
  ui::begin();
  ui::setBrightness(settings.brightness);
  ui::bootSplash();
  input_.begin();

  depth_ = 0;
  stack_[depth_++] = rootScreen();
  switchTo(current(), true);
}

void App::open(Screen* s) { pendingOpen_ = s; }
void App::back() { pendingBack_ = true; }

void App::switchTo(Screen* s, bool entering) {
  Serial.printf("[screen] %s depth=%d heap=%u\n", s->title(), depth_, ESP.getFreeHeap());
  ui::resetFocus(true);
  tft.fillScreen(Theme::BG);
  ui::drawHeader(s->title(), depth_ > 1);
  if (entering) s->onEnter();
  ui::drawNavigation();
}

// Stream the framebuffer over serial as raw RGB565 so the screen can be
// captured and viewed during development. Triggered by sending 'S'.
static void screenshot() {
  const int16_t w = tft.width(), h = tft.height();
  Serial.printf("SHOT %d %d\n", w, h);
  static uint16_t row[320];
  for (int16_t y = 0; y < h; y++) {
    tft.readRect(0, y, w, 1, row);
    Serial.write(reinterpret_cast<uint8_t*>(row), w * 2);
  }
  Serial.println();
  Serial.println("ENDSHOT");
}

void App::loop() {
  const uint32_t now = millis();

  InputEvent serialEvent;
  if (Serial.available()) {
    const int c = Serial.read();
    static uint8_t labDigits = 0, labIndex = 0;
    if (labDigits) {
      if (c >= '0' && c <= '9') {
        labIndex = labIndex * 10 + c - '0';
        if (--labDigits == 0 && labIndex < 26) open(advancedTool(labIndex));
      } else
        labDigits = 0;
    } else if (c == '!') {
      labDigits = 2;
      labIndex = 0;
    } else if (c == 'L')
      open(advancedLab());
    else if (c == 'U' || c == 'D' || c == 'N' || c == 'P' || c == 'E' || c == 'B') {
      Btn b = c == 'U'   ? Btn::Up
              : c == 'D' ? Btn::Down
              : c == 'N' ? Btn::Next
              : c == 'P' ? Btn::Prev
              : c == 'E' ? Btn::Select
                         : Btn::Back;
      serialEvent = {b, Evt::Press};
    } else if (c == 'V')
      Serial.printf("[status] version=%s uptime=%lu heap=%u color=%u touchCal=%u screen=%s\n",
                    FW_VERSION, (unsigned long)millis(), ESP.getFreeHeap(), settings.colorMode,
                    settings.touchCal, current() ? current()->title() : "none");
    else if (c == 'X')
      touch::debug = !touch::debug;
    else if (c == 'C') {
      settings.colorMode = (settings.colorMode + 1) % 8;
      ui::applyColorMode();
      settings.save();
      if (current()) {
        current()->onExit();
        switchTo(current(), true);
      }
    } else if (c == 'S')
      screenshot();       // dev: capture framebuffer
    else if (c == 'T') {  // dev: R|G|B calibration bars
      tft.fillRect(0, 0, 106, 240, TFT_RED);
      tft.fillRect(106, 0, 106, 240, TFT_GREEN);
      tft.fillRect(212, 0, 108, 240, TFT_BLUE);
    } else if (c == '1')
      open(&wifiScanScreen);  // dev: jump to a screen
    else if (c == '2')
      open(&wifiAuditScreen);
    else if (c == '3')
      open(&channelScreen);
    else if (c == '4')
      open(&deauthScreen);
    else if (c == '5')
      open(&bleScanScreen);
    else if (c == '6')
      open(&i2cScreen);
    else if (c == '7')
      open(&flashlightScreen);
    else if (c == '8')
      open(&sysInfoScreen);
    else if (c == '9')
      open(&settingsScreen);
    else if (c == 'a')
      open(&evilTwinScreen);
    else if (c == 'b')
      open(&packetMonitorScreen);
    else if (c == 'c')
      open(&bleRadarScreen);
    else if (c == 'd')
      open(&adcScreen);
    else if (c == 'e')
      open(&stopwatchScreen);
    else if (c == 'f')
      open(&snakeScreen);
    else if (c == 'g')
      open(&qrScreen);
    else if (c == 'h')
      open(&netToolsScreen);
    else if (c == 'i')
      open(sdFiles());
    else if (c == 'j')
      open(hiddenNetworks());
    else if (c == 'k')
      open(bleTracker());
    else if (c == 'l')
      open(clockScreen());
    else if (c == 'm')
      open(toolOptions());
    else if (c == 'n')
      open(beaconAudit());
    else if (c == 'o')
      open(serviceCheck());
    else if (c == 'p')
      open(dnsCheck());
    else if (c == 'J') {
      Serial.printf("[protocol-test] failures=%u\n", protocol::selfTest());
      Serial.printf("[lab-test] failures=%u\n", lab::selfTest());
    } else if (c == 'r')
      open(wifiWatch());
    else if (c == 's')
      open(bleInspector());
    else if (c == 't')
      open(httpCheck());
    else if (c == '0')
      back();
  }

  InputEvent e = input_.poll();
  if (serialEvent.btn != Btn::None) e = serialEvent;
  if (e.tap() && current() && current()->rawTouch()) e = {};
  if (e.btn != Btn::None && dismissAlarm()) e = {};
  if (e.tap() && e.y >= ui::H) {
    const Btn nav[] = {Btn::Up, Btn::Down, Btn::Select, Btn::Prev, Btn::Next, Btn::Back};
    e = {nav[min(5, ((int(e.x) + 1) * 6 - 1) / ui::W)], Evt::Press};
  }
  if (e.btn == Btn::Up || e.btn == Btn::Down || e.btn == Btn::Prev || e.btn == Btn::Next ||
      e.btn == Btn::Select) {
    if (current() && current()->onNavigation(e.btn))
      e = {};
    else
      e = ui::navigate(e.btn);
  }
  Screen* cur = current();
  if (cur) {
    if (e.is(Btn::Back, Evt::Press)) {
      back();
    } else if (e.tap()) {
      if (ui::backHit(e.x, e.y))
        back();
      else
        cur->onInput(e);
    }
  }

  // Input handlers can start timers and repaint; pass a fresh time afterward.
  if (cur) cur->update(millis());
  static uint32_t lastNav = 0;
  if (now - lastNav >= 200 || e.btn != Btn::None) {
    lastNav = now;
    ui::drawNavigation();
  }

  clockTick(now);

  // header clock once per second
  if (now - lastStatus_ >= 1000) {
    lastStatus_ = now;
    ui::headerClock(now);
  }

  // deferred navigation (so screens can call open/back from handlers)
  if (pendingBack_) {
    pendingBack_ = false;
    if (depth_ > 1) {
      stack_[--depth_]->onExit();
      switchTo(current(), false);
      // re-enter so the previous screen repaints its content
      current()->onEnter();
    }
  }
  if (pendingOpen_) {
    Screen* s = pendingOpen_;
    pendingOpen_ = nullptr;
    int found = -1;
    for (int i = 0; i < depth_; ++i)
      if (stack_[i] == s) found = i;
    if (found >= 0 && found < depth_ - 1) {
      current()->onExit();
      depth_ = found + 1;
      switchTo(current(), true);
    } else if (found < 0 && depth_ < MAX_DEPTH && s != current()) {
      if (current()) current()->onExit();
      stack_[depth_++] = s;
      switchTo(s, true);
    }
  }
}
