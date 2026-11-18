#include <SD.h>
#include "core/Storage.h"
#include "screens/Screen.h"
#include "ui/UI.h"
namespace {
class Files : public Screen {
  String path = "/", names[64], selected;
  bool dirs[64], preview = false, confirm = false;
  int n = 0, top = 0;
  size_t offset = 0, nextOffset = 0;
  String status;
  String full(int i) { return path == "/" ? path + names[i] : path + "/" + names[i]; }
  void list() {
    preview = confirm = false;
    n = 0;
    status = "";
    if (storage::mount()) {
      File dir = SD.open(path);
      if (dir && dir.isDirectory()) {
        while (n < 64) {
          File f = dir.openNextFile();
          if (!f) break;
          String name = f.name();
          int pos = name.lastIndexOf('/');
          if (pos >= 0) name = name.substring(pos + 1);
          names[n] = name;
          dirs[n++] = f.isDirectory();
          f.close();
        }
        dir.close();
      } else
        status = "تعذر فتح المجلد";
    } else
      status = storage::lastError();
    top = constrain(top, 0, max(0, n - 1));
    draw();
  }
  void draw() {
    ui::clearContent(true);
    txt::draw(8, 33, path.c_str(), Theme::MID, Theme::BG, txt::LEFT, txt::FONT_S);
    if (status.length())
      txt::draw(160, 82, status.c_str(), Theme::WARN, Theme::BG, txt::CENTER, txt::FONT_S);
    for (int j = 0; j < 4 && top + j < n; ++j) {
      int i = top + j, y = 52 + j * 31;
      ui::tile(8, y, 304, 29, false);
      ui::addFocus(8, y, 304, 29);
      String label = (dirs[i] ? "[DIR] " : "") + names[i];
      txt::draw(16, y + 6, label.c_str(), Theme::FG, Theme::TILE, txt::LEFT, txt::FONT_S);
    }
    const char* f[] = {"اعلى", "اسفل", "المجلد الاب", "تحديث"};
    ui::footerBar(f, nullptr, 4);
  }
  void read() {
    ui::clearContent(true);
    File f = SD.open(selected);
    if (!f) {
      ui::toast("فشل فتح الملف");
      return;
    }
    f.seek(offset);
    for (int row = 0; row < 7 && f.available(); ++row) {
      String line;
      for (int col = 0; col < 40 && f.available(); ++col) {
        char c = f.read();
        if (c == '\n') break;
        if (c == '\r') continue;
        line += ((uint8_t)c >= 32 && (uint8_t)c < 127) ? c : '.';
      }
      txt::draw(8, 35 + row * 20, line.c_str(), Theme::FG, Theme::BG, txt::LEFT, txt::FONT_S);
    }
    nextOffset = f.position();
    f.close();
    const char* ftr[] = {"من البداية", "التالي", "حذف", "قائمة"};
    ui::footerBar(ftr, nullptr, 4);
  }

 public:
  const char* title() const override { return "مدير ملفات SD"; }
  void onEnter() override { list(); }
  bool onInput(const InputEvent& e) override {
    if (!e.tap()) return false;
    if (confirm) {
      int f = ui::footerHit(e.x, e.y, 2);
      if (f == 0) {
        bool ok = SD.remove(selected);
        preview = false;
        list();
        ui::toast(ok ? "تم الحذف" : "فشل الحذف");
      } else if (f == 1) {
        confirm = false;
        read();
      }
      return true;
    }
    if (preview) {
      int f = ui::footerHit(e.x, e.y, 4);
      if (f == 0) {
        offset = 0;
        read();
      }
      if (f == 1) {
        offset = nextOffset;
        read();
      }
      if (f == 2) {
        confirm = true;
        ui::clearContent();
        ui::toast("حذف الملف المحدد؟", Theme::WARN);
        const char* ftr[] = {"تأكيد الحذف", "الغاء"};
        ui::footerBar(ftr, nullptr, 2);
      }
      if (f == 3) {
        preview = false;
        list();
      }
      return true;
    }
    int f = ui::footerHit(e.x, e.y, 4);
    if (f >= 0) {
      if (f == 0) top = max(0, top - 4);
      if (f == 1 && top + 4 < n) top += 4;
      if (f == 2) {
        int p = path.lastIndexOf('/');
        path = p > 0 ? path.substring(0, p) : "/";
        top = 0;
      }
      list();
      return true;
    }
    if (e.y >= 52 && e.y < 176) {
      int i = top + (e.y - 52) / 31;
      if (i < n) {
        if (dirs[i]) {
          path = full(i);
          top = 0;
          list();
        } else {
          selected = full(i);
          offset = 0;
          preview = true;
          read();
        }
      }
      return true;
    }
    return false;
  }
} files;
}  // namespace
Screen* sdFiles() { return &files; }
