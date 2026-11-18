#include "ui/UI.h"
#include "config.h"
#include "core/Settings.h"

TFT_eSPI tft;

namespace ui {
namespace {
constexpr int BL_CH = 0;
#ifdef TFT_BL
constexpr int BL_PIN = TFT_BL;
#else
constexpr int BL_PIN = -1;
#endif
bool canBack_ = false;
struct Hit {
  int16_t x, y, w, h;
};
Hit hits[64];
int hitCount = 0, focus = 0;

void vgrad(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t top, uint16_t bot) {
  for (int16_t i = 0; i < h; i++)
    tft.drawFastHLine(x, y + i, w, Theme::lerp(top, bot, (uint8_t)(255 * i / (h - 1))));
}
}  // namespace

void begin() {
  tft.init();
  tft.setRotation(DISPLAY_ROTATION);
  applyColorMode();
  tft.fillScreen(Theme::BG);
  tft.setTextWrap(false);
  txt::begin(&tft);
  if (BL_PIN >= 0) {
    ledcSetup(BL_CH, 5000, 8);
    ledcAttachPin(BL_PIN, BL_CH);
    ledcWrite(BL_CH, 255);
  }
}

void applyColorMode() {
  Theme::byteSwap = settings.colorMode & 2;
  Theme::applyTheme(settings.theme);
#if defined(BOARD_CYD)
  // Rotation 3 on ILI9341: MY|MX|MV. Only the colour-order bit changes.
  tft.writecommand(0x36);
  tft.writedata(0xE0 | ((settings.colorMode & 1) ? 0x08 : 0));
#endif
  tft.invertDisplay(settings.colorMode & 4);
}
void resetFocus(bool home) {
  hitCount = 0;
  if (home) focus = 0;
}
void addFocus(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (y < CONTENT_Y || y + h > H || w <= 0 || h <= 0) return;
  for (int i = 0; i < hitCount; ++i)
    if (hits[i].x == x && hits[i].y == y) return;
  if (hitCount < 64) hits[hitCount++] = {x, y, w, h};
}
void drawNavigation() {
  const char* labels[] = {"اعلى", "اسفل", "اختيار", "السابق", "التالي", "رجوع"};
  tft.fillRect(0, H, W, PHYSICAL_H - H, Theme::BG);
  for (int i = 0; i < 6; ++i) {
    int x = i * W / 6, w = (i + 1) * W / 6 - x;
    tft.drawRect(x, H, w, 28, Theme::EDGE);
    txt::draw(x + w / 2, H + 7, labels[i], Theme::FG, Theme::BG, txt::CENTER, txt::FONT_S);
  }
  if (hitCount) {
    focus = constrain(focus, 0, hitCount - 1);
    const auto& r = hits[focus];
    tft.drawRect(r.x, r.y, r.w, r.h, Theme::ACCENT);
  }
}
InputEvent navigate(Btn b) {
  if (!hitCount) return {};
  focus = constrain(focus, 0, hitCount - 1);
  if (b == Btn::Select) {
    const auto& r = hits[focus];
    return {Btn::Tap, Evt::Press, (int16_t)(r.x + r.w / 2), (int16_t)(r.y + r.h / 2)};
  }
  const auto old = hits[focus];
  tft.drawRect(old.x, old.y, old.w, old.h, Theme::EDGE);
  int step = (b == Btn::Up || b == Btn::Prev) ? -1 : 1;
  int next = (focus + step + hitCount) % hitCount;
  if (b == Btn::Up || b == Btn::Down) {
    int best = 1000000;
    for (int i = 0; i < hitCount; ++i) {
      int dy = (hits[i].y + hits[i].h / 2) - (old.y + old.h / 2);
      int dx = abs((hits[i].x + hits[i].w / 2) - (old.x + old.w / 2));
      if ((b == Btn::Up && dy < 0) || (b == Btn::Down && dy > 0)) {
        int score = abs(dy) * 4 + dx;
        if (score < best) {
          best = score;
          next = i;
        }
      }
    }
  }
  focus = next;
  drawNavigation();
  return {};
}

void setBrightness(uint8_t v) {
  if (BL_PIN >= 0) ledcWrite(BL_CH, v < 12 ? 12 : v);
}

// --------------------------------------------------------------- splash ----
void bootSplash() {
  tft.fillScreen(Theme::BG);
  const int16_t cx = W / 2, cy = 96;
  for (int r = 6; r <= 70; r += 8) {
    tft.drawCircle(cx, cy, r, Theme::lerp(Theme::FG, Theme::BG, (uint8_t)min(255, r * 3)));
    delay(16);
  }
  tft.drawCircle(cx, cy, 70, Theme::DIM);
  icon::draw(tft, icon::SHIELD, cx, cy, 30, Theme::FG);
  txt::draw(cx, cy + 44, "CYBERCORE", Theme::ACCENT, Theme::BG, txt::CENTER, txt::FONT_M);
  txt::draw(cx, cy + 68, "اداة امن الشبكات", Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
  const int16_t bx = 50, bw = W - 100, by = cy + 92;
  tft.drawRoundRect(bx - 2, by - 2, bw + 4, 10, 3, Theme::EDGE);
  for (int i = 0; i <= bw; i += 4) {
    tft.fillRect(bx, by, i, 6, Theme::lerp(Theme::DIM, Theme::FG, (uint8_t)(255 * i / bw)));
    delay(8);
  }
  delay(220);
}

// --------------------------------------------------------------- header ----
void drawHeader(const char* titleAr, bool canBack) {
  canBack_ = canBack;
  vgrad(0, 0, W, HEADER_H, Theme::HDR_A, Theme::HDR_B);
  // accent underline with a notch
  tft.drawFastHLine(0, HEADER_H - 1, W, Theme::EDGE);
  tft.drawFastHLine(0, HEADER_H - 2, W, Theme::lerp(Theme::ACCENT, Theme::BG, 150));
  tft.fillRect(0, HEADER_H - 2, 64, 2, Theme::ACCENT);

  if (canBack) {
    icon::draw(tft, icon::BACK, 16, HEADER_H / 2, 7, Theme::ACCENT);
    tft.drawFastVLine(32, 5, HEADER_H - 10, Theme::EDGE);
  }
  // title (right-aligned, Arabic)
  txt::draw(W - 10, 5, titleAr, Theme::FG, Theme::HDR_A, txt::RIGHT, txt::FONT_M);
}

bool backHit(int16_t x, int16_t y) { return canBack_ && x < 40 && y < HEADER_H; }

void headerClock(uint32_t ms) {
  const uint32_t s = ms / 1000;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)((s / 60) % 100),
           (unsigned long)(s % 60));
  const int16_t x = canBack_ ? 42 : 8;
  const uint16_t bg = Theme::lerp(Theme::HDR_A, Theme::HDR_B, 150);
  tft.fillRect(x, 6, 40, HEADER_H - 12, bg);
  txt::draw(x, 8, buf, Theme::MID, bg, txt::LEFT, txt::FONT_S);
}

void clearContent(bool withFooter) {
  resetFocus();
  tft.fillRect(0, CONTENT_Y, W, withFooter ? CONTENT_HB : CONTENT_H, Theme::BG);
}

// ---------------------------------------------------------------- tiles ----
void glowRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
  tft.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 7, Theme::lerp(c, Theme::BG, 120));
  tft.drawRoundRect(x, y, w, h, 6, c);
}

void tile(int16_t x, int16_t y, int16_t w, int16_t h, bool pressed) {
  tft.fillRoundRect(x, y, w, h, 6, pressed ? Theme::TILE_SEL : Theme::TILE);
  if (pressed)
    glowRect(x, y, w, h, Theme::GLOW);
  else
    tft.drawRoundRect(x, y, w, h, 6, Theme::EDGE);
  // corner accents
  tft.drawFastHLine(x + 6, y, 12, Theme::ACCENT);
  tft.drawFastHLine(x + w - 18, y + h - 1, 12, Theme::lerp(Theme::ACCENT, Theme::BG, 90));
}

// --------------------------------------------------------------- footer ----
void footerBar(const char* const* labels, const icon::Id* icons, int n, int pressed) {
  vgrad(0, FOOTER_Y, W, FOOTER_H, Theme::HDR_B, Theme::HDR_A);
  tft.drawFastHLine(0, FOOTER_Y, W, Theme::EDGE);
  const int16_t bw = W / n;
  for (int i = 0; i < n; ++i) addFocus(i * bw, FOOTER_Y, bw, FOOTER_H);
  for (int i = 0; i < n; i++) {
    const int16_t bx = i * bw;
    if (i == pressed) tft.fillRect(bx + 1, FOOTER_Y + 1, bw - 2, FOOTER_H - 1, Theme::TILE_SEL);
    if (i) tft.drawFastVLine(bx, FOOTER_Y + 4, FOOTER_H - 8, Theme::EDGE);
    const uint16_t c = (i == pressed) ? Theme::ACCENT : Theme::MID;
    int16_t tx = bx + bw / 2;
    if (icons) {
      icon::draw(tft, icons[i], bx + 16, FOOTER_Y + FOOTER_H / 2, 7, c);
      tx = bx + 30;
    }
    txt::draw(tx + (txt::width(labels[i], txt::FONT_S) / 2), FOOTER_Y + 6, labels[i], c,
              Theme::HDR_A, txt::RIGHT, txt::FONT_S);
  }
}

int footerHit(int16_t x, int16_t y, int n) {
  if (n <= 0 || x < 0 || x >= W || y < FOOTER_Y || y >= H) return -1;
  return min(n - 1, x / (W / n));
}

void toast(const char* ar, uint16_t color) {
  const int16_t w = txt::width(ar, txt::FONT_M) + 28, x = (W - w) / 2, y = H / 2 - 16;
  tft.fillRoundRect(x, y, w, 32, 6, Theme::TILE_SEL);
  glowRect(x, y, w, 32, color);
  txt::draw(x + w - 14, y + 5, ar, color, Theme::TILE_SEL, txt::RIGHT, txt::FONT_M);
}

}  // namespace ui
