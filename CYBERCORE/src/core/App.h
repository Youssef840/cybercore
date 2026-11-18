#pragma once
// ============================================================================
//  App - boot, main loop, and a screen navigation stack (push/pop for submenus)
// ============================================================================
#include "core/Input.h"
#include "screens/Screen.h"

class App {
 public:
  static App& instance();

  void begin();
  void loop();

  void open(Screen* s);   // push + show
  void back();            // pop to previous (no-op at root)
  Screen* current() const { return depth_ ? stack_[depth_ - 1] : nullptr; }

 private:
  App() = default;
  void switchTo(Screen* s, bool entering);

  static constexpr int MAX_DEPTH = 8;
  Input input_;
  Screen* stack_[MAX_DEPTH];
  int depth_ = 0;
  Screen* pendingOpen_ = nullptr;
  bool pendingBack_ = false;
  uint32_t lastStatus_ = 0;
};
