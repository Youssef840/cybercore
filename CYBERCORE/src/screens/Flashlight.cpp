#include "core/App.h"
#include "core/Settings.h"
#include "screens/Modules.h"
#include "ui/UI.h"

void FlashlightScreen::onEnter() {
  ui::setBrightness(255);
  tft.fillScreen(TFT_WHITE);
  txt::draw(ui::W / 2, ui::H - 22, "المس للخروج", 0x7BEF, TFT_WHITE, txt::CENTER, txt::FONT_S);
}

void FlashlightScreen::onExit() { ui::setBrightness(settings.brightness); }

bool FlashlightScreen::onInput(const InputEvent& e) {
  if (e.tap()) { App::instance().back(); return true; }
  return false;
}
