#pragma once
// ============================================================================
//  Grid - icon-tile menu. Direct tap opens a tile's target.
// ============================================================================
#include "screens/Screen.h"
#include "ui/Icons.h"

struct GridItem {
  const char* label;  // Arabic
  icon::Id icon;
  Screen* target;
};

class Grid : public Screen {
 public:
  Grid(const char* titleAr, const GridItem* items, int n) : title_(titleAr), items_(items), n_(n) {}
  const char* title() const override { return title_; }
  void onEnter() override;
  bool onInput(const InputEvent& e) override;

 private:
  void rect(int i, int16_t& x, int16_t& y, int16_t& w, int16_t& h);
  void drawTile(int i, bool pressed);
  const char* title_;
  const GridItem* items_;
  int n_;
  int page_ = 0;
};
