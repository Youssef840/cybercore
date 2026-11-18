#include "screens/Modules.h"
#include "ui/UI.h"

// Classic snake. Tap a side of the board (relative to the head) to steer.
namespace {
constexpr int16_t CELL = 11;
const char* const kFoot[] = {"اعادة"};
int16_t BX, BY;  // board origin
}  // namespace

void SnakeScreen::reset() {
  BX = (ui::W - GW * CELL) / 2;
  BY = ui::CONTENT_Y + (ui::CONTENT_HB - GH * CELL) / 2;
  len_ = 3;
  dir_ = 0;
  score_ = 0;
  dead_ = paused_ = false;
  for (int i = 0; i < len_; i++) {
    sx_[i] = 5 - i;
    sy_[i] = GH / 2;
  }
  place();
}

void SnakeScreen::place() {
  bool ok;
  do {
    ok = true;
    fx_ = random(GW);
    fy_ = random(GH);
    for (int i = 0; i < len_; i++)
      if (sx_[i] == fx_ && sy_[i] == fy_) ok = false;
  } while (!ok);
}

void SnakeScreen::onEnter() {
  ui::clearContent(true);
  randomSeed(micros());
  reset();
  ui::footerBar(kFoot, nullptr, 1);
}

void SnakeScreen::update(uint32_t now) {
  if (dead_ || paused_ || now - last_ < 170) return;
  last_ = now;

  int nx = sx_[0] + (dir_ == 0 ? 1 : dir_ == 2 ? -1 : 0);
  int ny = sy_[0] + (dir_ == 1 ? 1 : dir_ == 3 ? -1 : 0);
  if (nx < 0 || nx >= GW || ny < 0 || ny >= GH) {
    dead_ = true;
  }
  for (int i = 0; i < len_ && !dead_; i++)
    if (sx_[i] == nx && sy_[i] == ny) dead_ = true;

  if (dead_) {
    txt::draw(ui::W / 2, ui::CONTENT_Y + 50, "انتهت اللعبة", Theme::ALERT, Theme::BG, txt::CENTER,
              txt::FONT_M);
    char b[24];
    snprintf(b, sizeof(b), "النقاط: %d", score_);
    txt::draw(ui::W / 2, ui::CONTENT_Y + 76, b, Theme::FG, Theme::BG, txt::CENTER, txt::FONT_S);
    return;
  }

  const bool grew = (nx == fx_ && ny == fy_);
  const int tailOld = len_;
  if (grew) {
    len_ = min(len_ + 1, GW * GH);
    score_++;
  }
  // erase old tail cell
  if (!grew) {
    tft.fillRect(BX + sx_[tailOld - 1] * CELL, BY + sy_[tailOld - 1] * CELL, CELL, CELL, Theme::BG);
  }
  for (int i = len_ - 1; i > 0; i--) {
    sx_[i] = sx_[i - 1];
    sy_[i] = sy_[i - 1];
  }
  sx_[0] = nx;
  sy_[0] = ny;
  if (grew) place();

  // draw food + head
  tft.fillRect(BX + fx_ * CELL + 2, BY + fy_ * CELL + 2, CELL - 4, CELL - 4, Theme::ALERT);
  tft.fillRect(BX + nx * CELL + 1, BY + ny * CELL + 1, CELL - 2, CELL - 2, Theme::FG);
  tft.fillRect(BX + sx_[1] * CELL + 1, BY + sy_[1] * CELL + 1, CELL - 2, CELL - 2, Theme::MID);

  // score header
  char b[16];
  snprintf(b, sizeof(b), "%d", score_);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(Theme::ACCENT, Theme::BG);
  tft.fillRect(60, 6, 40, 16, Theme::HDR_A);
  tft.drawString(b, 62, 14, 2);
}

bool SnakeScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  if (ui::footerHit(e.x, e.y, 1) == 0) {
    onEnter();
    return true;
  }
  // steer: direction from tap position relative to head cell
  const int16_t hx = BX + sx_[0] * CELL + CELL / 2, hy = BY + sy_[0] * CELL + CELL / 2;
  const int dx = e.x - hx, dy = e.y - hy;
  if (abs(dx) > abs(dy)) {
    if (dx > 0 && dir_ != 2)
      dir_ = 0;
    else if (dx < 0 && dir_ != 0)
      dir_ = 2;
  } else {
    if (dy > 0 && dir_ != 3)
      dir_ = 1;
    else if (dy < 0 && dir_ != 1)
      dir_ = 3;
  }
  return true;
}

bool SnakeScreen::onNavigation(Btn b) {
  if (b == Btn::Select) {
    if (dead_)
      onEnter();
    else
      paused_ = !paused_;
    return true;
  }
  int d = b == Btn::Up ? 3 : b == Btn::Down ? 1 : b == Btn::Prev ? 2 : b == Btn::Next ? 0 : -1;
  if (d < 0) return false;
  if ((d + 2) % 4 != dir_) dir_ = d;
  return true;
}
