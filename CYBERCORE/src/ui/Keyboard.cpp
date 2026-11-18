#include "ui/Keyboard.h"
#include "ui/UI.h"

namespace kb {
namespace {
char buf[64];
uint8_t len = 0;
bool mask_ = false;
const char* title_ = "";
uint8_t layer = 0;  // 0 lower, 1 upper, 2 symbols

// 4 character rows per layer (row lengths 10,10,9,7)
const char* ROWS[3][4] = {
    {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"},
    {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"},
    {"1234567890", "@#$_&-+()/", "*\"':;!?=%", ".,~|{}[]"},
};

constexpr int16_t KY = ui::CONTENT_Y + 40;  // first key row top
constexpr int16_t KH = 23, KW = 30, GAP = 2;
constexpr int16_t FROW = KY + 4 * (KH + GAP);  // function row

int16_t rowX(int n) { return (ui::W - (n * (KW + GAP) - GAP)) / 2; }

void key(int16_t x, int16_t y, int16_t w, const char* label, bool hot) {
  ui::addFocus(x, y, w, KH);
  tft.fillRoundRect(x, y, w, KH, 4, hot ? Theme::TILE_SEL : Theme::TILE);
  tft.drawRoundRect(x, y, w, KH, 4, hot ? Theme::ACCENT : Theme::EDGE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(hot ? Theme::ACCENT : Theme::FG, hot ? Theme::TILE_SEL : Theme::TILE);
  tft.drawString(label, x + w / 2, y + KH / 2, 2);
}
}  // namespace

void open(const char* title, bool mask, const char* initial) {
  title_ = title;
  mask_ = mask;
  layer = 0;
  strncpy(buf, initial ? initial : "", sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  len = strlen(buf);
  draw();
}

const char* text() { return buf; }

void draw() {
  ui::clearContent();
  // title + field
  txt::draw(ui::W - 10, ui::CONTENT_Y + 2, title_, Theme::MID, Theme::BG, txt::RIGHT, txt::FONT_S);
  tft.drawRoundRect(10, ui::CONTENT_Y + 18, ui::W - 20, 20, 3, Theme::EDGE);
  char show[64];
  if (mask_) {
    memset(show, '*', len);
    show[len] = 0;
  } else
    strcpy(show, buf);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(Theme::FG, Theme::BG);
  tft.setTextPadding(ui::W - 30);
  tft.drawString(show, 16, ui::CONTENT_Y + 28, 2);
  tft.setTextPadding(0);

  // character rows
  char s[2] = {0, 0};
  for (int r = 0; r < 4; r++) {
    const char* row = ROWS[layer][r];
    const int n = strlen(row);
    int16_t x = rowX(n), y = KY + r * (KH + GAP);
    for (int i = 0; i < n; i++) {
      s[0] = row[i];
      key(x, y, KW, s, false);
      x += KW + GAP;
    }
  }
  // function row: [123/abc] [cancel] [space] [back] [ok]
  const int16_t sp = ui::W - 2 * 6;
  key(6, FROW, 44, layer == 2 ? "abc" : "123", layer == 2);
  key(54, FROW, 40, "X", false);
  key(98, FROW, sp - 98 - 94, "___", false);  // space
  key(ui::W - 92, FROW, 40, "<-", false);
  key(ui::W - 48, FROW, 42, "OK", true);
}

Result tap(int16_t x, int16_t y) {
  // character rows
  for (int r = 0; r < 4; r++) {
    const int16_t ry = KY + r * (KH + GAP);
    if (y >= ry && y < ry + KH) {
      const char* row = ROWS[layer][r];
      const int n = strlen(row);
      int16_t rx = rowX(n);
      if (x >= rx && x < rx + n * (KW + GAP)) {
        const int idx = (x - rx) / (KW + GAP);
        if (idx >= 0 && idx < n && len < sizeof(buf) - 1) {
          buf[len++] = row[idx];
          buf[len] = 0;
          if (layer == 1) layer = 0;  // one-shot caps
          draw();
        }
        return TYPING;
      }
    }
  }
  // function row
  if (y >= FROW && y < FROW + KH) {
    if (x < 50) {
      layer = layer == 2 ? 0 : 2;
      draw();
      return TYPING;
    }  // 123/abc
    if (x < 94) return CANCEL;                // X
    if (x >= ui::W - 92 && x < ui::W - 50) {  // back
      if (len) {
        buf[--len] = 0;
        draw();
      }
      return TYPING;
    }
    if (x >= ui::W - 48) return DONE;  // OK
    buf[len < sizeof(buf) - 1 ? len++ : len] = ' ';
    buf[len] = 0;
    draw();  // space
    return TYPING;
  }
  // tap the field area toggles caps as a shift shortcut
  if (y < KY) {
    layer = layer == 0 ? 1 : 0;
    draw();
  }
  return NONE;
}
}  // namespace kb
