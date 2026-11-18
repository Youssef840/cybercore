#include "ui/Font.h"
#include "ui/Theme.h"
#include "ui/ArabicText.h"
#include "ui/font_m.h"  // defines FONT_M
#include "ui/font_s.h"  // defines FONT_S

namespace txt {
namespace {
TFT_eSPI* gTft = nullptr;

const AGlyph* find(const AFont& f, uint16_t cp) {
  int lo = 0, hi = f.count - 1;
  while (lo <= hi) {
    const int mid = (lo + hi) >> 1;
    const uint16_t c = f.glyphs[mid].cp;
    if (c == cp) return &f.glyphs[mid];
    if (c < cp)
      lo = mid + 1;
    else
      hi = mid - 1;
  }
  return nullptr;
}

int16_t advanceOf(const AFont& f, uint16_t cp) {
  if (cp == ' ') return f.space;
  const AGlyph* g = find(f, cp);
  return g ? g->adv : f.space;
}

void blit(const AFont& f, const AGlyph* g, int16_t x, int16_t y, uint16_t fg, uint16_t bg) {
  if (!g || !g->w || !g->h) return;
  const uint8_t* bm = &f.bitmap[g->off];
  const int16_t ox = x + g->xo, oy = y + g->yo;
  uint32_t idx = 0;
  for (uint8_t row = 0; row < g->h; row++) {
    for (uint8_t col = 0; col < g->w; col++, idx++) {
      const uint8_t byte = bm[idx >> 1];
      const uint8_t nib = (idx & 1) ? (byte & 0x0F) : (byte >> 4);
      if (!nib) continue;
      const uint16_t c = (nib >= 15) ? fg : Theme::lerp(bg, fg, (uint8_t)(nib * 17));
      gTft->drawPixel(ox + col, oy + row, c);
    }
  }
}
}  // namespace

void begin(TFT_eSPI* tft) { gTft = tft; }

int16_t lineHeight(const AFont& f) { return f.height; }

int16_t width(const char* utf8, const AFont& f) {
  const arabic::Shaped s = arabic::shape(utf8);
  int16_t w = 0;
  for (uint16_t i = 0; i < s.len; i++) w += advanceOf(f, s.glyphs[i]);
  return w;
}

int16_t draw(int16_t x, int16_t y, const char* utf8, uint16_t fg, uint16_t bg, Align align,
             const AFont& f) {
  const arabic::Shaped s = arabic::shape(utf8);
  int16_t total = 0;
  for (uint16_t i = 0; i < s.len; i++) total += advanceOf(f, s.glyphs[i]);

  int16_t pen = x;
  if (align == CENTER)
    pen = x - total / 2;
  else if (align == RIGHT)
    pen = x - total;

  for (uint16_t i = 0; i < s.len; i++) {
    const uint16_t cp = s.glyphs[i];
    if (cp == ' ') {
      pen += f.space;
      continue;
    }
    const AGlyph* g = find(f, cp);
    if (g) {
      blit(f, g, pen, y, fg, bg);
      pen += g->adv;
    } else
      pen += f.space;
  }
  return total;
}
}  // namespace txt
