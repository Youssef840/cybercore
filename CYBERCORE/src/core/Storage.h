#pragma once
#include <Arduino.h>
namespace storage {
bool mount();
String quote(const char* value);
bool append(const char* kind, const String& line, const char* header);
const char* lastError();
}  // namespace storage
