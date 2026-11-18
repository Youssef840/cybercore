#pragma once
// ============================================================================
//  Theme - runtime palette. Core hues are variables so themes can switch live.
//  WARN/ALERT/MAGENTA stay fixed (status colours). Default = green/black.
// ============================================================================
#include <Arduino.h>

namespace Theme {
constexpr uint16_t swap16(uint16_t c) { return (uint16_t)((c >> 8) | (c << 8)); }

extern bool byteSwap;
inline uint16_t encode(uint16_t c) { return byteSwap ? swap16(c) : c; }
inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return encode((uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)));
}
inline uint16_t lerp(uint16_t a, uint16_t b, uint8_t t) {
  a = encode(a);
  b = encode(b);
  int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
  int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
  int r = ar + (br - ar) * t / 255, g = ag + (bg - ag) * t / 255, bl = ab + (bb - ab) * t / 255;
  return encode((uint16_t)((r << 11) | (g << 5) | bl));
}

// Runtime palette (set by applyTheme)
extern uint16_t BG, FG, MID, DIM, GRID, PANEL, ACCENT, GLOW, EDGE, HDR_A, HDR_B, TILE, TILE_SEL;

// Fixed status colours
extern uint16_t WARN, ALERT, MAGENTA;

constexpr uint8_t THEME_COUNT = 4;
void applyTheme(uint8_t idx);
const char* themeName(uint8_t idx);
uint16_t themeBase(uint8_t idx);  // swatch colour
}  // namespace Theme
