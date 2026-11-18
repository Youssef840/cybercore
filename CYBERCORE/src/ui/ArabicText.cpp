#include "ui/ArabicText.h"

namespace arabic {

namespace {
// ---- Positional-form table for base letters U+0621..U+064A ----------------
//  iso = isolated, fin = final, ini = initial, med = medial (0 = no such form)
//  dual = true  -> connects on BOTH sides (has initial/medial forms)
//  dual = false -> connects only to the PREVIOUS letter (alef, dal, ra, waw..)
struct Form {
  uint16_t iso, fin, ini, med;
  bool dual;
};

// Index = codepoint - 0x0621. 0x064A - 0x0621 + 1 = 42 entries.
const Form kForms[] = {
    {0xFE80, 0, 0, 0, false},                 // 0621 hamza (non-joining)
    {0xFE81, 0xFE82, 0, 0, false},            // 0622 alef madda
    {0xFE83, 0xFE84, 0, 0, false},            // 0623 alef hamza above
    {0xFE85, 0xFE86, 0, 0, false},            // 0624 waw hamza
    {0xFE87, 0xFE88, 0, 0, false},            // 0625 alef hamza below
    {0xFE89, 0xFE8A, 0xFE8B, 0xFE8C, true},   // 0626 yeh hamza
    {0xFE8D, 0xFE8E, 0, 0, false},            // 0627 alef
    {0xFE8F, 0xFE90, 0xFE91, 0xFE92, true},   // 0628 beh
    {0xFE93, 0xFE94, 0, 0, false},            // 0629 teh marbuta
    {0xFE95, 0xFE96, 0xFE97, 0xFE98, true},   // 062A teh
    {0xFE99, 0xFE9A, 0xFE9B, 0xFE9C, true},   // 062B theh
    {0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0, true},   // 062C jeem
    {0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4, true},   // 062D hah
    {0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8, true},   // 062E khah
    {0xFEA9, 0xFEAA, 0, 0, false},            // 062F dal
    {0xFEAB, 0xFEAC, 0, 0, false},            // 0630 thal
    {0xFEAD, 0xFEAE, 0, 0, false},            // 0631 reh
    {0xFEAF, 0xFEB0, 0, 0, false},            // 0632 zain
    {0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4, true},   // 0633 seen
    {0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8, true},   // 0634 sheen
    {0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC, true},   // 0635 sad
    {0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0, true},   // 0636 dad
    {0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4, true},   // 0637 tah
    {0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8, true},   // 0638 zah
    {0xFEC9, 0xFECA, 0xFECB, 0xFECC, true},   // 0639 ain
    {0xFECD, 0xFECE, 0xFECF, 0xFED0, true},   // 063A ghain
    {0, 0, 0, 0, false},                      // 063B (gap)
    {0, 0, 0, 0, false},                      // 063C
    {0, 0, 0, 0, false},                      // 063D
    {0, 0, 0, 0, false},                      // 063E
    {0, 0, 0, 0, false},                      // 063F
    {0, 0, 0, 0, false},                      // 0640 tatweel (kashida)
    {0xFED1, 0xFED2, 0xFED3, 0xFED4, true},   // 0641 feh
    {0xFED5, 0xFED6, 0xFED7, 0xFED8, true},   // 0642 qaf
    {0xFED9, 0xFEDA, 0xFEDB, 0xFEDC, true},   // 0643 kaf
    {0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0, true},   // 0644 lam
    {0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4, true},   // 0645 meem
    {0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8, true},   // 0646 noon
    {0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC, true},   // 0647 heh
    {0xFEED, 0xFEEE, 0, 0, false},            // 0648 waw
    {0xFEEF, 0xFEF0, 0, 0, false},            // 0649 alef maksura
    {0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4, true},   // 064A yeh
};
constexpr uint32_t BASE_LO = 0x0621, BASE_HI = 0x064A;

inline bool isArabicLetter(uint32_t cp) { return cp >= BASE_LO && cp <= BASE_HI; }
inline const Form* formOf(uint32_t cp) {
  return isArabicLetter(cp) ? &kForms[cp - BASE_LO] : nullptr;
}
// Diacritics (tashkeel) + tatweel: ignored, and transparent to joining.
inline bool isTransparent(uint32_t cp) {
  return (cp >= 0x064B && cp <= 0x065F) || cp == 0x0640 || cp == 0x0670;
}
// A letter can be joined FROM its right (i.e. the previous letter can attach):
// every Arabic letter except the plain hamza.
inline bool joinsToPrev(uint32_t cp) { return isArabicLetter(cp) && cp != 0x0621; }
// A letter can attach to the NEXT letter: only dual-joining letters.
inline bool joinsToNext(uint32_t cp) {
  const Form* f = formOf(cp);
  return f && f->dual;
}

// lam (0644) + alef variant -> presentation ligature. Returns iso/fin pair.
bool lamAlef(uint32_t lam, uint32_t next, uint16_t* iso, uint16_t* fin) {
  if (lam != 0x0644) return false;
  switch (next) {
    case 0x0622: *iso = 0xFEF5; *fin = 0xFEF6; return true;  // laa with madda
    case 0x0623: *iso = 0xFEF7; *fin = 0xFEF8; return true;  // laa hamza above
    case 0x0625: *iso = 0xFEF9; *fin = 0xFEFA; return true;  // laa hamza below
    case 0x0627: *iso = 0xFEFB; *fin = 0xFEFC; return true;  // laa plain
    default: return false;
  }
}

inline bool isNeutral(uint32_t cp) {  // stays attached to the surrounding run
  return cp == ' ' || cp == 0 || isTransparent(cp);
}
}  // namespace

uint32_t nextCodepoint(const char* s, size_t* i) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
  uint8_t c = p[*i];
  if (c == 0) return 0;
  if (c < 0x80) { (*i)++; return c; }
  if ((c & 0xE0) == 0xC0) {
    uint32_t cp = ((c & 0x1F) << 6) | (p[*i + 1] & 0x3F);
    *i += 2;
    return cp;
  }
  if ((c & 0xF0) == 0xE0) {
    uint32_t cp = ((c & 0x0F) << 12) | ((p[*i + 1] & 0x3F) << 6) | (p[*i + 2] & 0x3F);
    *i += 3;
    return cp;
  }
  if ((c & 0xF8) == 0xF0) {
    uint32_t cp = ((c & 0x07) << 18) | ((p[*i + 1] & 0x3F) << 12) | ((p[*i + 2] & 0x3F) << 6) |
                  (p[*i + 3] & 0x3F);
    *i += 4;
    return cp;
  }
  (*i)++;
  return c;  // invalid byte, pass through
}

bool hasArabic(const char* utf8) {
  size_t i = 0;
  uint32_t cp;
  while ((cp = nextCodepoint(utf8, &i)))
    if (isArabicLetter(cp) || isTransparent(cp)) return true;
  return false;
}

Shaped shape(const char* utf8) {
  // ---- 1. decode to codepoints (dropping transparent marks) ----
  uint32_t cps[MAX_GLYPHS];
  uint16_t n = 0;
  size_t i = 0;
  uint32_t cp;
  while ((cp = nextCodepoint(utf8, &i)) && n < MAX_GLYPHS) {
    if (isTransparent(cp)) continue;  // drop diacritics for a clean bitmap
    cps[n++] = cp;
  }

  // ---- 2. shape into logical-order glyphs ----
  uint16_t logical[MAX_GLYPHS];
  uint16_t m = 0;
  for (uint16_t k = 0; k < n && m < MAX_GLYPHS; k++) {
    const uint32_t c = cps[k];
    const Form* f = formOf(c);
    if (!f) { logical[m++] = (uint16_t)c; continue; }  // non-Arabic passthrough

    // lam-alef ligature
    uint16_t liso, lfin;
    if (c == 0x0644 && k + 1 < n && lamAlef(c, cps[k + 1], &liso, &lfin)) {
      const bool prevJoin = (k > 0) && joinsToNext(cps[k - 1]);
      logical[m++] = prevJoin ? lfin : liso;
      k++;  // consume the alef
      continue;
    }

    const bool prevJoin = (k > 0) && joinsToNext(cps[k - 1]);
    const bool nextJoin = f->dual && (k + 1 < n) && joinsToPrev(cps[k + 1]);

    uint16_t g;
    if (prevJoin && nextJoin)      g = f->med;
    else if (prevJoin)             g = f->fin;
    else if (nextJoin)             g = f->ini;
    else                           g = f->iso;
    if (g == 0) g = f->iso;        // safety
    logical[m++] = g;
  }

  // ---- 3. bidi reorder: reverse the whole line, but keep contiguous
  //         non-Arabic (Latin/digits/spaces inside them) runs internally LTR.
  Shaped out;
  for (int16_t k = (int16_t)m - 1; k >= 0; k--) {
    const uint16_t g = logical[k];
    const bool arab = (g >= 0xFE70 && g <= 0xFEFF);
    if (arab) {
      out.glyphs[out.len++] = g;
    } else {
      // find the start of this non-Arabic run (scanning left)
      int16_t s = k;
      while (s > 0) {
        const uint16_t gp = logical[s - 1];
        if (gp >= 0xFE70 && gp <= 0xFEFF) break;
        s--;
      }
      for (int16_t j = s; j <= k && out.len < MAX_GLYPHS; j++) out.glyphs[out.len++] = logical[j];
      k = s;  // loop's k-- moves past the run
    }
    if (out.len >= MAX_GLYPHS) break;
  }
  return out;
}

}  // namespace arabic
