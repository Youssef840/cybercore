// Standalone Arabic-rendering proof. Built only by env:cyd2usb_fonttest.
#ifdef AR_FONT_TEST
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "ui/Font.h"

static TFT_eSPI tft;

static constexpr uint16_t BG  = 0x0000;              // black
static constexpr uint16_t GRN = 0x07E6;              // phosphor green
static constexpr uint16_t CYN = 0x07FF;              // cyan
static constexpr uint16_t DIM = 0x03E0;              // dim green
static constexpr uint16_t WHT = 0xFFFF;

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("[fonttest] Arabic render proof");

  tft.init();
  tft.setRotation(DISPLAY_ROTATION);
  tft.fillScreen(BG);
  txt::begin(&tft);

  const int16_t W = tft.width();
  const int16_t R = W - 8;        // right margin (Arabic is right-aligned)
  int16_t y = 6;
  const int16_t step = txt::lineHeight() + 2;

  tft.drawFastHLine(0, 0, W, DIM);

  txt::draw(R, y, "سايبر كور - اختبار الخط", CYN, BG, txt::RIGHT);  y += step;
  txt::draw(R, y, "القائمة الرئيسية", GRN, BG, txt::RIGHT);          y += step;
  txt::draw(R, y, "ماسح شبكات الواي فاي", GRN, BG, txt::RIGHT);      y += step;
  txt::draw(R, y, "تدقيق الحماية: محمي ضد الفصل", GRN, BG, txt::RIGHT); y += step;
  txt::draw(R, y, "لا إله إلا الله", WHT, BG, txt::RIGHT);           y += step;
  txt::draw(R, y, "القناة ٦  -  الإشارة ٨٥٪", DIM, BG, txt::RIGHT);  y += step;
  txt::draw(R, y, "WiFi: 12 شبكة قريبة", GRN, BG, txt::RIGHT);       y += step;

  // Left-aligned + centred samples to check alignment
  tft.drawFastHLine(0, y + 2, W, DIM);
  txt::draw(8, y + 6, "يسار", DIM, BG, txt::LEFT);
  txt::draw(W / 2, y + 6, "وسط", CYN, BG, txt::CENTER);

  Serial.println("[fonttest] drawn");
}

void loop() { delay(1000); }
#endif  // AR_FONT_TEST
