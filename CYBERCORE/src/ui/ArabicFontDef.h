#pragma once
// Shared types for the generated Arabic bitmap fonts (font_s.h / font_m.h).
#include <Arduino.h>

struct AGlyph {
  uint16_t cp;
  uint8_t w, h, adv;
  int8_t xo, yo;
  uint16_t off;
};

struct AFont {
  uint8_t height, ascent, space;
  uint16_t count;
  const AGlyph* glyphs;
  const uint8_t* bitmap;
};
