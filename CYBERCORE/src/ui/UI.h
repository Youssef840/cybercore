#pragma once
// ============================================================================
//  UI - premium Arabic chrome: gradient status bar, icon tiles, footer buttons.
//  Landscape 320x240 (rotation 3). Direct-touch; BOOT = back.
// ============================================================================
#include <TFT_eSPI.h>
#include "core/Input.h"
#include "ui/Font.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

extern TFT_eSPI tft;

namespace ui {
constexpr int16_t W = 320;
constexpr int16_t H = 212;
constexpr int16_t PHYSICAL_H = 240;
constexpr int16_t HEADER_H = 30;
constexpr int16_t FOOTER_H = 28;
constexpr int16_t FOOTER_Y = H - FOOTER_H;
constexpr int16_t CONTENT_Y = HEADER_H + 2;
constexpr int16_t CONTENT_H = H - HEADER_H - 2;              // full (no footer)
constexpr int16_t CONTENT_HB = H - HEADER_H - FOOTER_H - 2;  // with footer

void begin();
void applyColorMode();
void resetFocus(bool home = false);
void addFocus(int16_t x, int16_t y, int16_t w, int16_t h);
InputEvent navigate(Btn button);
void drawNavigation();
void setBrightness(uint8_t v);
void bootSplash();

// Header / chrome
void drawHeader(const char* titleAr, bool canBack);
bool backHit(int16_t x, int16_t y);  // tap landed on the back chevron
void headerClock(uint32_t uptimeMs);
void clearContent(bool withFooter = false);

// Tiles & panels
void tile(int16_t x, int16_t y, int16_t w, int16_t h, bool pressed);
void glowRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);

// Footer buttons (tappable). Up to 4. Returns index hit or -1.
void footerBar(const char* const* labels, const icon::Id* icons, int n, int pressed = -1);
int footerHit(int16_t x, int16_t y, int n);

// Transient centred message
void toast(const char* ar, uint16_t color = Theme::ACCENT);
}  // namespace ui
