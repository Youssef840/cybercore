#include "core/Touch.h"
#include "core/Settings.h"
#include "screens/Screen.h"
#include "ui/UI.h"
#include <algorithm>
namespace {
class Calibration : public Screen {
  int step = 0, samples = 0;
  bool released = true, waitRelease = true;
  uint32_t last = 0;
  uint16_t xs[15], ys[15], rx[3], ry[3];
  const int px[3] = {25, 294, 25}, py[3] = {48, 48, 187};
  void draw() {
    ui::clearContent();
    txt::draw(160, 32, "اضغط العلامة بالقلم ثم ارفع", Theme::FG, Theme::BG, txt::CENTER,
              txt::FONT_S);
    tft.drawCircle(px[step], py[step], 10, Theme::ACCENT);
    tft.drawLine(px[step] - 14, py[step], px[step] + 14, py[step], Theme::FG);
    tft.drawLine(px[step], py[step] - 14, px[step], py[step] + 14, Theme::FG);
  }

 public:
  bool rawTouch() const override { return true; }
  const char* title() const override { return "معايرة اللمس"; }
  void onEnter() override {
    step = samples = 0;
    released = false;
    waitRelease = true;
    draw();
  }
  bool onInput(const InputEvent&) override { return true; }
  void update(uint32_t now) override {
    if (now - last < 25 || step >= 3) return;
    last = now;
    auto p = touch::read();
    if (!p.pressed) {
      released = true;
      if (samples) samples = 0;
      return;
    }
    if (waitRelease) {
      if (!released) return;
      waitRelease = false;
      released = false;
    }
    xs[samples] = p.rawX;
    ys[samples] = p.rawY;
    if (++samples < 15) return;
    std::sort(xs, xs + 15);
    std::sort(ys, ys + 15);
    rx[step] = xs[7];
    ry[step] = ys[7];
    samples = 0;
    step++;
    waitRelease = true;
    released = false;
    if (step < 3) {
      draw();
      return;
    }
    bool sw = abs((int)ry[1] - ry[0]) > abs((int)rx[1] - rx[0]);
    int x0 = sw ? ry[0] : rx[0], x1 = sw ? ry[1] : rx[1], y0 = sw ? rx[0] : ry[0],
        y1 = sw ? rx[2] : ry[2];
    if (abs(x1 - x0) < 600 || abs(y1 - y0) < 600) {
      step = 0;
      draw();
      ui::toast("اعد المحاولة", Theme::WARN);
      return;
    }
    settings.touchX0 = x0 - (x1 - x0) * 25 / 269;
    settings.touchX1 = x0 + (x1 - x0) * 294 / 269;
    settings.touchY0 = y0 - (y1 - y0) * 48 / 139;
    settings.touchY1 = y0 + (y1 - y0) * 191 / 139;
    settings.touchSwap = sw;
    settings.touchCal = true;
    settings.save();
    ui::clearContent();
    ui::toast("تمت المعايرة - رجوع");
  }
} calibration;
}  // namespace
Screen* touchCalibration() { return &calibration; }
