#include "qrcode.h"
#include "core/Storage.h"
#include "screens/Modules.h"
#include "ui/Keyboard.h"
#include "ui/UI.h"
void QrScreen::onEnter() {
  editing_ = false;
  draw();
}
void QrScreen::draw() {
  ui::clearContent(true);
  QRCode qr;
  static uint8_t buf[256];
  if (qrcode_initText(&qr, buf, 4, ECC_LOW, text_) != 0) {
    ui::toast("تعذر توليد الرمز", Theme::WARN);
    return;
  }
  const int scale = 3, qsz = (qr.size + 8) * scale, ox = (ui::W - qsz) / 2, oy = 36;
  tft.fillRect(ox, oy, qsz, qsz, Theme::rgb(255, 255, 255));
  for (int y = 0; y < qr.size; ++y)
    for (int x = 0; x < qr.size; ++x)
      if (qrcode_getModule(&qr, x, y))
        tft.fillRect(ox + (x + 4) * scale, oy + (y + 4) * scale, scale, scale, Theme::rgb(0, 0, 0));
  String hint = String(strlen(text_)) + " / 63 bytes";
  txt::draw(160, 165, hint.c_str(), Theme::MID, Theme::BG, txt::CENTER, txt::FONT_S);
  const char* f[] = {"تعديل النص", "حفظ SD"};
  ui::footerBar(f, nullptr, 2);
}
bool QrScreen::onInput(const InputEvent& e) {
  if (!e.tap()) return false;
  if (editing_) {
    auto result = kb::tap(e.x, e.y);
    if (result == kb::DONE) {
      strlcpy(text_, kb::text(), sizeof(text_));
      editing_ = false;
      draw();
    } else if (result == kb::CANCEL) {
      editing_ = false;
      draw();
    }
    return true;
  }
  int f = ui::footerHit(e.x, e.y, 2);
  if (f == 0) {
    editing_ = true;
    kb::open("نص الرمز", false, text_);
  }
  if (f == 1) {
    bool ok = storage::append("qr_text", String(millis()) + "," + storage::quote(text_),
                              "uptime_ms,text");
    ui::toast(ok ? "تم الحفظ" : "فشل الحفظ SD");
  }
  return true;
}
