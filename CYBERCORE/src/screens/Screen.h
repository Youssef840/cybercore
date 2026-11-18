#pragma once
// ============================================================================
//  Screen - base class for every page. Titles/labels are Arabic UTF-8.
// ============================================================================
#include <Arduino.h>
#include "core/Input.h"

class Screen {
 public:
  virtual ~Screen() = default;
  virtual const char* title() const = 0;  // Arabic, shown in header
  virtual void onEnter() {}
  virtual void onExit() {}
  virtual bool onInput(const InputEvent& e) = 0;  // true = handled
  virtual void update(uint32_t now) {}
  virtual bool onNavigation(Btn button) { return false; }
  virtual bool rawTouch() const { return false; }
};
