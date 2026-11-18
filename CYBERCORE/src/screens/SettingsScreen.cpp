#include "config.h"
#include "core/Settings.h"
#include "core/App.h"
#include "screens/Modules.h"
#include "ui/UI.h"
extern Screen* touchCalibration();
extern Screen* toolOptions();
namespace {
const char* modes[] = {"RGB",     "BGR",     "RGB SWAP",     "BGR SWAP",
                       "RGB INV", "BGR INV", "RGB SWAP INV", "BGR SWAP INV"};
}
void SettingsScreen::onEnter() { draw(); }
void SettingsScreen::onExit() { settings.save(); }
void SettingsScreen::footer(int) {
  const char* f[] = {"حفظ", "معايرة اللمس", "خيارات الادوات"};
  ui::footerBar(f, nullptr, 3);
}
void SettingsScreen::draw() {
  ui::clearContent(true);
  const char* labels[] = {"سطوع اقل", "سطوع اكثر", "الثيم", "ترتيب الالوان"};
  for (int i = 0; i < 4; ++i) {
    int y = ui::CONTENT_Y + i * 29;
    ui::tile(8, y, 304, 27, false);
    ui::addFocus(8, y, 304, 27);
    txt::draw(300, y + 5, labels[i], Theme::FG, Theme::TILE, txt::RIGHT, txt::FONT_S);
    String value = i < 2    ? String(settings.brightness * 100 / 255) + "%"
                   : i == 2 ? Theme::themeName(settings.theme)
                            : modes[settings.colorMode];
    txt::draw(16, y + 5, value.c_str(), Theme::ACCENT, Theme::TILE, txt::LEFT, txt::FONT_S);
  }
  const uint16_t colors[] = {Theme::rgb(255, 0, 0), Theme::rgb(0, 255, 0), Theme::rgb(0, 0, 255),
                             Theme::rgb(255, 255, 255), Theme::rgb(0, 0, 0)};
  const char* names[] = {"R", "G", "B", "W", "K"};
  for (int i = 0; i < 5; ++i) {
    int x = 12 + i * 60;
    tft.fillRect(x, 151, 54, 24, colors[i]);
    tft.drawRect(x, 151, 54, 24, Theme::MID);
    txt::draw(x + 27, 156, names[i], i == 4 ? Theme::rgb(255, 255, 255) : Theme::rgb(0, 0, 0),
              colors[i], txt::CENTER, txt::FONT_S);
  }
  footer();
}
bool SettingsScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  int f = ui::footerHit(e.x, e.y, 3);
  if (f == 0) {
    settings.save();
    ui::toast("تم الحفظ");
    msg_ = millis();
    return true;
  }
  if (f == 1) {
    App::instance().open(touchCalibration());
    return true;
  }
  if (f == 2) {
    App::instance().open(toolOptions());
    return true;
  }
  int row = (e.y - ui::CONTENT_Y) / 29;
  if (e.y < ui::CONTENT_Y || row > 3) return false;
  if (row < 2) {
    settings.brightness = constrain(settings.brightness + (row ? 16 : -16), 12, 255);
    ui::setBrightness(settings.brightness);
  }
  if (row == 2) {
    settings.theme = (settings.theme + 1) % Theme::THEME_COUNT;
    Theme::applyTheme(settings.theme);
  }
  if (row == 3) {
    settings.colorMode = (settings.colorMode + 1) % 8;
    ui::applyColorMode();
  }
  ui::drawHeader(title(), true);
  draw();
  return true;
}
void SettingsScreen::update(uint32_t now) {
  if (msg_ && now - msg_ > 1000) {
    msg_ = 0;
    draw();
  }
}
