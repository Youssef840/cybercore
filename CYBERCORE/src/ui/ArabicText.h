#pragma once
// ============================================================================
//  ArabicText - Arabic shaping + bidi for displays that draw left-to-right.
//
//  TFT_eSPI (and almost every embedded font renderer) cannot shape Arabic:
//  it draws codepoints one by one, left to right, with no letter joining and
//  no right-to-left ordering. This module fixes that in software:
//
//    1. SHAPING  : each Arabic letter is replaced by its correct positional
//                  form (isolated / initial / medial / final) from the
//                  Unicode "Arabic Presentation Forms-B" block (U+FE70..FEFF),
//                  including the lam-alef ligatures.
//    2. BIDI      : the logical string is reordered for visual display -
//                  Arabic runs are reversed, Latin/number runs keep their
//                  order ("poor man's bidi", correct for UI labels).
//
//  Input  : UTF-8 (Arabic + ASCII mixed).
//  Output : an array of uint16_t glyph codepoints, already in visual order,
//           ready to be drawn left-to-right by a font that covers the
//           Presentation Forms-B block.
// ============================================================================
#include <Arduino.h>

namespace arabic {

constexpr size_t MAX_GLYPHS = 128;

struct Shaped {
  uint16_t glyphs[MAX_GLYPHS];
  uint16_t len = 0;
};

// Shape + reorder a UTF-8 string into visual-order glyph codepoints.
Shaped shape(const char* utf8);

// Convenience: true if the string contains any Arabic codepoint.
bool hasArabic(const char* utf8);

// Decode one UTF-8 sequence; advances *i. Returns the codepoint (0 on end).
uint32_t nextCodepoint(const char* s, size_t* i);

}  // namespace arabic
