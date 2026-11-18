#include "screens/Grid.h"
#include "core/App.h"
#include "ui/UI.h"

namespace {
constexpr int16_t PAD = 8, GAP = 8;
}

void Grid::rect(int i, int16_t& x, int16_t& y, int16_t& w, int16_t& h) {
  const int cols = (n_ == 1) ? 1 : (n_ > 6 ? 3 : 2);
  const int rows = (min(n_, 6) + cols - 1) / cols;
  i %= 6;
  const int16_t y0 = ui::CONTENT_Y + 6;
  const int16_t availW = ui::W - 2 * PAD;
  const int16_t availH = (n_ > 6 ? ui::FOOTER_Y : ui::H) - y0 - PAD;
  w = (availW - (cols - 1) * GAP) / cols;
  h = (availH - (rows - 1) * GAP) / rows;
  const int col = i % cols, row = i / cols;
  x = PAD + col * (w + GAP);
  y = y0 + row * (h + GAP);
}

void Grid::drawTile(int i, bool pressed) {
  int16_t x, y, w, h;
  rect(i, x, y, w, h);
  ui::tile(x, y, w, h, pressed);
  ui::addFocus(x, y, w, h);
  const bool small = (n_ > 4);
  const int16_t r = min(w, h) * (small ? 0.22f : 0.26f);
  const uint16_t ic = pressed ? Theme::ACCENT : Theme::FG;
  icon::draw(tft, items_[i].icon, x + w / 2, y + (int16_t)(h * 0.31f), r, ic);
  txt::draw(x + w / 2, y + h - (small ? 23 : 30), items_[i].label,
            pressed ? Theme::ACCENT : Theme::FG, pressed ? Theme::TILE_SEL : Theme::TILE,
            txt::CENTER, small ? txt::FONT_S : txt::FONT_M);
}

void Grid::onEnter() {
  ui::clearContent();
  for (int i = page_ * 6; i < min(n_, page_ * 6 + 6); i++) drawTile(i, false);
  if (n_ > 6) {
    const char* labels[] = {"الصفحة السابقة", "الصفحة التالية"};
    ui::footerBar(labels, nullptr, 2);
  }
}

bool Grid::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  if (n_ > 6 && e.y >= ui::FOOTER_Y) {
    int f = ui::footerHit(e.x, e.y, 2);
    if (f >= 0) {
      page_ = (page_ + (f ? 1 : (n_ + 5) / 6 - 1)) % ((n_ + 5) / 6);
      onEnter();
      return true;
    }
  }
  for (int i = page_ * 6; i < min(n_, page_ * 6 + 6); i++) {
    int16_t x, y, w, h;
    rect(i, x, y, w, h);
    if (e.x >= x && e.x < x + w && e.y >= y && e.y < y + h) {
      drawTile(i, true);
      delay(110);
      if (items_[i].target)
        App::instance().open(items_[i].target);
      else
        drawTile(i, false);
      return true;
    }
  }
  return false;
}
