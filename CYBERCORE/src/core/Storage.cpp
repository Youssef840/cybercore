#include "core/Storage.h"
#include "config.h"
#include <SD.h>
#include <SPI.h>
namespace storage {
namespace {
SPIClass sdBus(VSPI);
bool mounted = false;
const char* error = "";
}  // namespace
const char* lastError() { return error; }
bool mount() {
  if (mounted && SD.cardType() != CARD_NONE) return true;
  if (PIN_SD_CS == PIN_NONE) {
    error = "SD غير متاح";
    return false;
  }
  pinMode(PIN_CC_CS, OUTPUT);
  digitalWrite(PIN_CC_CS, HIGH);
  sdBus.begin(PIN_CC_SCK, PIN_CC_MISO, PIN_CC_MOSI, PIN_SD_CS);
  mounted = SD.begin(PIN_SD_CS, sdBus, 4000000);
  if (!mounted || SD.cardType() == CARD_NONE) {
    mounted = false;
    error = "تعذر قراءة بطاقة SD";
    Serial.println("[sd] unavailable");
    return false;
  }
  Serial.printf("[sd] mounted size_mb=%llu\n", SD.cardSize() / (1024 * 1024));
  error = "";
  return true;
}
String quote(const char* value) {
  String out = "\"";
  if (value && (*value == '=' || *value == '+' || *value == '-' || *value == '@')) out += '\'';
  if (value)
    for (const char* p = value; *p; ++p) {
      if (*p == '\"') out += '\"';
      out += *p;
    }
  return out + '\"';
}
bool append(const char* kind, const String& line, const char* header) {
  if (!mount()) return false;
  if (!SD.exists("/CYBERCORE") && !SD.mkdir("/CYBERCORE")) {
    error = "فشل انشاء المجلد";
    return false;
  }
  String path = String("/CYBERCORE/") + kind + ".csv";
  bool fresh = !SD.exists(path);
  File f = SD.open(path, FILE_APPEND);
  if (!f) {
    error = "فشل فتح الملف";
    return false;
  }
  bool ok = true;
  if (fresh) ok = f.println(header) > 0;
  ok = (f.println(line) > 0) && ok;
  f.flush();
  f.close();
  if (!ok) error = "فشل حفظ النتائج";
  return ok;
}
}  // namespace storage
