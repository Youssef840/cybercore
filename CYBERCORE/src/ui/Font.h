#pragma once
// ============================================================================
//  Font - shaped Arabic + ASCII text from the generated bitmap fonts.
//  Two sizes: FONT_S (small - labels, footer, status) and FONT_M (headings).
// ============================================================================
#include <TFT_eSPI.h>
#include "ui/ArabicFontDef.h"

namespace txt {
enum Align : uint8_t { LEFT, CENTER, RIGHT };

extern const AFont FONT_S, FONT_M;   // defined in Font.cpp

void begin(TFT_eSPI* tft);

int16_t width(const char* utf8, const AFont& f = FONT_M);
int16_t lineHeight(const AFont& f = FONT_M);

// Draw with the text TOP at y; x is the anchor per align. bg = colour the text
// sits on (for anti-alias blending). Returns the drawn width.
int16_t draw(int16_t x, int16_t y, const char* utf8, uint16_t fg, uint16_t bg,
             Align align = RIGHT, const AFont& f = FONT_M);
}  // namespace txt
