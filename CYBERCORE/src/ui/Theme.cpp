#include "ui/Theme.h"

namespace Theme {
bool byteSwap = false;
uint16_t WARN, ALERT, MAGENTA;
uint16_t BG, FG, MID, DIM, GRID, PANEL, ACCENT, GLOW, EDGE, HDR_A, HDR_B, TILE, TILE_SEL;

namespace {
struct Preset {
  uint16_t base, accent;
  const char* name;
};
const Preset P[THEME_COUNT] = {
    {rgb(0, 160, 45), rgb(60, 230, 70), "هكر"},       // deep phosphor green on black
    {rgb(40, 255, 120), rgb(180, 255, 210), "اخضر"},  // bright green
    {rgb(0, 200, 255), rgb(150, 245, 255), "سماوي"},
    {rgb(255, 120, 0), rgb(255, 220, 120), "كهرماني"},
};
}  // namespace

void applyTheme(uint8_t idx) {
  if (idx >= THEME_COUNT) idx = 0;
  WARN = rgb(255, 220, 0);
  ALERT = rgb(255, 40, 70);
  MAGENTA = rgb(255, 0, 170);
  const uint16_t b = encode(P[idx].base);
  BG = rgb(0, 0, 0);
  FG = b;
  MID = lerp(BG, b, 170);
  DIM = lerp(BG, b, 95);
  GRID = lerp(BG, b, 48);
  PANEL = lerp(BG, b, 22);
  EDGE = lerp(BG, b, 80);
  GLOW = lerp(BG, b, 150);
  TILE = lerp(BG, b, 18);
  TILE_SEL = lerp(BG, b, 50);
  HDR_A = lerp(BG, b, 42);
  HDR_B = lerp(BG, b, 10);
  ACCENT = encode(P[idx].accent);
}

const char* themeName(uint8_t idx) { return P[idx < THEME_COUNT ? idx : 0].name; }
uint16_t themeBase(uint8_t idx) { return encode(P[idx < THEME_COUNT ? idx : 0].base); }
}  // namespace Theme
