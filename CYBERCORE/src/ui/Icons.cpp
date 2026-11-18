#include "ui/Icons.h"
#include "ui/Theme.h"

namespace icon {
namespace {
void wifi(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  // drawSmoothArc: 0deg = 6 o'clock, so "up" is around 180deg.
  const int16_t oy = cy + (int16_t)(r * 0.5f);    // fan origin (bottom dot)
  const int rad[3] = {(int)(r * 1.05f), (int)(r * 0.72f), (int)(r * 0.40f)};
  for (int i = 0; i < 3; i++) {
    const int th = 3;
    t.drawSmoothArc(cx, oy, rad[i], rad[i] - th, 122, 180, c, Theme::BG, true);
    t.drawSmoothArc(cx, oy, rad[i], rad[i] - th, 180, 238, c, Theme::BG, true);
  }
  t.fillCircle(cx, oy, 3, c);
}

void bluetooth(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t top = cy - r, bot = cy + r, mid = cy;
  const int16_t x0 = cx, xr = cx + (int16_t)(r * 0.6f);
  // spine
  t.drawWideLine(x0, top, x0, bot, 2, c, Theme::BG);
  // upper and lower triangles to the right
  t.drawWideLine(x0, top, xr, cy - (int16_t)(r * 0.5f), 2, c, Theme::BG);
  t.drawWideLine(xr, cy - (int16_t)(r * 0.5f), x0, mid, 2, c, Theme::BG);
  t.drawWideLine(x0, bot, xr, cy + (int16_t)(r * 0.5f), 2, c, Theme::BG);
  t.drawWideLine(xr, cy + (int16_t)(r * 0.5f), x0, mid, 2, c, Theme::BG);
  // left nubs
  t.drawWideLine(cx - (int16_t)(r * 0.55f), cy - (int16_t)(r * 0.3f),
                 cx + (int16_t)(r * 0.25f), cy + (int16_t)(r * 0.2f), 2, c, Theme::BG);
  t.drawWideLine(cx - (int16_t)(r * 0.55f), cy + (int16_t)(r * 0.3f),
                 cx + (int16_t)(r * 0.25f), cy - (int16_t)(r * 0.2f), 2, c, Theme::BG);
}

void chip(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t s = (int16_t)(r * 0.85f);
  t.drawRoundRect(cx - s, cy - s, 2 * s, 2 * s, 3, c);
  t.drawRect(cx - s / 2, cy - s / 2, s, s, c);
  for (int i = -1; i <= 1; i++) {
    const int16_t p = i * (s / 2);
    t.drawFastVLine(cx + p, cy - s - 4, 4, c);   // top pins
    t.drawFastVLine(cx + p, cy + s + 1, 4, c);   // bottom
    t.drawFastHLine(cx - s - 4, cy + p, 4, c);   // left
    t.drawFastHLine(cx + s + 1, cy + p, 4, c);   // right
  }
}

void gear(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  for (int a = 0; a < 360; a += 45) {
    const float rad = a * 0.0174533f;
    const int16_t x0 = cx + cosf(rad) * (r * 0.6f), y0 = cy + sinf(rad) * (r * 0.6f);
    const int16_t x1 = cx + cosf(rad) * r, y1 = cy + sinf(rad) * r;
    t.drawWideLine(x0, y0, x1, y1, 4, c, Theme::BG);
  }
  t.fillCircle(cx, cy, r * 0.55f, c);
  t.fillCircle(cx, cy, r * 0.25f, Theme::BG);
}

void shield(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t top = cy - r, w = (int16_t)(r * 0.85f);
  t.drawWideLine(cx - w, top + 3, cx, top, 2, c, Theme::BG);
  t.drawWideLine(cx, top, cx + w, top + 3, 2, c, Theme::BG);
  t.drawWideLine(cx - w, top + 3, cx - w, cy, 2, c, Theme::BG);
  t.drawWideLine(cx + w, top + 3, cx + w, cy, 2, c, Theme::BG);
  t.drawWideLine(cx - w, cy, cx, cy + r, 2, c, Theme::BG);
  t.drawWideLine(cx + w, cy, cx, cy + r, 2, c, Theme::BG);
  // check mark
  t.drawWideLine(cx - w / 2, cy, cx - w / 6, cy + r / 3, 3, Theme::ACCENT, Theme::BG);
  t.drawWideLine(cx - w / 6, cy + r / 3, cx + w / 2, cy - r / 3, 3, Theme::ACCENT, Theme::BG);
}

void radar(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.drawCircle(cx, cy, r, c);
  t.drawCircle(cx, cy, (int)(r * 0.62f), Theme::lerp(c, Theme::BG, 110));
  t.drawCircle(cx, cy, (int)(r * 0.28f), Theme::lerp(c, Theme::BG, 110));
  t.drawWideLine(cx, cy, cx + (int16_t)(r * 0.9f), cy - (int16_t)(r * 0.5f), 2, Theme::ACCENT,
                 Theme::BG);
  t.fillCircle(cx + (int16_t)(r * 0.55f), cy - (int16_t)(r * 0.3f), 2, Theme::ACCENT);
}

void lock(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t w = (int16_t)(r * 1.1f), h = (int16_t)(r * 0.8f);
  t.fillRoundRect(cx - w / 2, cy - h / 4, w, h, 2, c);
  t.drawSmoothArc(cx, cy - h / 4, (int)(r * 0.5f), (int)(r * 0.5f) - 2, 270, 90, c, Theme::BG, true);
  t.fillCircle(cx, cy + h / 8, 2, Theme::BG);
}

void apps(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t s = r - 1, g = r / 2 + 1;
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      t.fillRoundRect(cx - r + i * (s + g) - (i ? 0 : 0), cy - r + j * (s + g), s, s, 2, c);
}

void eye(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.drawSmoothArc(cx, cy + r, 2 * r, 2 * r - 2, 300, 360, c, Theme::BG, true);
  t.drawSmoothArc(cx, cy + r, 2 * r, 2 * r - 2, 0, 60, c, Theme::BG, true);
  t.drawSmoothArc(cx, cy - r, 2 * r, 2 * r - 2, 120, 180, c, Theme::BG, true);
  t.drawSmoothArc(cx, cy - r, 2 * r, 2 * r - 2, 180, 240, c, Theme::BG, true);
  t.fillCircle(cx, cy, r / 2 + 1, c);
}

void bolt(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.fillTriangle(cx + r / 3, cy - r, cx - r / 2, cy + r / 6, cx, cy + r / 6, c);
  t.fillTriangle(cx - r / 3, cy + r, cx + r / 2, cy - r / 6, cx, cy - r / 6, c);
}

void timer(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.fillRect(cx - r / 3, cy - r - 4, 2 * r / 3, 3, c);     // top button
  t.drawCircle(cx, cy, r, c);
  t.drawCircle(cx, cy, r - 1, c);
  t.drawWideLine(cx, cy, cx, cy - r * 0.6f, 2, c, Theme::BG);
  t.drawWideLine(cx, cy, cx + r * 0.4f, cy, 2, c, Theme::BG);
}

void qr(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t x0 = cx - r, y0 = cy - r, u = r / 2;
  auto finder = [&](int16_t x, int16_t y) {
    t.drawRect(x, y, u, u, c);
    t.fillRect(x + 2, y + 2, u - 4, u - 4, c);
  };
  finder(x0, y0);
  finder(x0 + r + u, y0);
  finder(x0, y0 + r + u);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if ((i + j) & 1) t.fillRect(x0 + r + u / 2 + i * 4, y0 + r + u / 2 + j * 4, 3, 3, c);
}

void game(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.drawRoundRect(cx - r, cy - r / 2, 2 * r, r, r / 2, c);
  // d-pad
  t.fillRect(cx - r / 2 - 2, cy - 2, 9, 4, c);
  t.fillRect(cx - r / 2 + 1, cy - 5, 4, 10, c);
  // buttons
  t.fillCircle(cx + r / 2, cy - 2, 2, c);
  t.fillCircle(cx + r / 2 - 6, cy + 3, 2, c);
}

void back(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.drawWideLine(cx + r / 2, cy - r, cx - r / 2, cy, 3, c, Theme::BG);
  t.drawWideLine(cx - r / 2, cy, cx + r / 2, cy + r, 3, c, Theme::BG);
}

void refresh(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.drawSmoothArc(cx, cy, r, r - 2, 40, 320, c, Theme::BG, true);
  t.fillTriangle(cx + r - 2, cy - r + 2, cx + r + 3, cy - 2, cx - 1, cy - r + 1, c);
}

void graph(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t h[4] = {(int16_t)(r * 0.5f), (int16_t)(r * 1.0f), (int16_t)(r * 0.7f),
                        (int16_t)(r * 1.3f)};
  const int16_t bw = r / 2, x0 = cx - r;
  for (int i = 0; i < 4; i++)
    t.fillRect(x0 + i * (bw + 2), cy + r - h[i], bw, h[i], c);
  t.drawFastHLine(x0 - 2, cy + r + 1, r * 2 + 6, Theme::lerp(c, Theme::BG, 110));
}

void alert(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.drawWideLine(cx, cy - r, cx - r, cy + r, 2, c, Theme::BG);
  t.drawWideLine(cx, cy - r, cx + r, cy + r, 2, c, Theme::BG);
  t.drawWideLine(cx - r, cy + r, cx + r, cy + r, 2, c, Theme::BG);
  t.drawWideLine(cx, cy - r / 3, cx, cy + r / 3, 3, c, Theme::BG);
  t.fillCircle(cx, cy + r * 0.62f, 2, c);
}

void info(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  t.drawCircle(cx, cy, r, c);
  t.drawCircle(cx, cy, r - 1, c);
  t.fillCircle(cx, cy - r / 2, 2, c);
  t.fillRect(cx - 1, cy - r / 6, 3, r / 2 + r / 6, c);
}

void light(TFT_eSPI& t, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  const int16_t br = (int16_t)(r * 0.62f);
  t.fillCircle(cx, cy - r / 4, br, c);
  t.fillCircle(cx, cy - r / 4, br - 3, Theme::BG);
  t.fillRect(cx - br / 2, cy + r / 3, br, r / 3, c);   // base
  for (int a = 0; a < 360; a += 45) {                  // rays
    const float rad = a * 0.0174533f;
    t.drawWideLine(cx + cosf(rad) * (r * 0.8f), cy - r / 4 + sinf(rad) * (r * 0.8f),
                   cx + cosf(rad) * r, cy - r / 4 + sinf(rad) * r, 2, c, Theme::BG);
  }
}
}  // namespace

void draw(TFT_eSPI& t, Id id, int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  switch (id) {
    case WIFI: wifi(t, cx, cy, r, c); break;
    case BLUETOOTH: bluetooth(t, cx, cy, r, c); break;
    case CHIP: chip(t, cx, cy, r, c); break;
    case GEAR: gear(t, cx, cy, r, c); break;
    case SHIELD: shield(t, cx, cy, r, c); break;
    case RADAR: radar(t, cx, cy, r, c); break;
    case LOCK: lock(t, cx, cy, r, c); break;
    case BACK: back(t, cx, cy, r, c); break;
    case REFRESH: refresh(t, cx, cy, r, c); break;
    case GRAPH: graph(t, cx, cy, r, c); break;
    case ALERT: alert(t, cx, cy, r, c); break;
    case LIGHT: light(t, cx, cy, r, c); break;
    case INFO: info(t, cx, cy, r, c); break;
    case APPS: apps(t, cx, cy, r, c); break;
    case EYE: eye(t, cx, cy, r, c); break;
    case BOLT: bolt(t, cx, cy, r, c); break;
    case TIMER: timer(t, cx, cy, r, c); break;
    case QR: qr(t, cx, cy, r, c); break;
    case GAME: game(t, cx, cy, r, c); break;
  }
}
}  // namespace icon
