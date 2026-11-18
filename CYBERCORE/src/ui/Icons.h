#pragma once
// ============================================================================
//  Icons - crisp vector icons drawn with TFT primitives (scalable, no bitmaps)
// ============================================================================
#include <TFT_eSPI.h>

namespace icon {
enum Id : uint8_t { WIFI, BLUETOOTH, CHIP, GEAR, SHIELD, RADAR, LOCK, BACK, REFRESH,
                    GRAPH, ALERT, LIGHT, INFO, APPS, EYE, BOLT, TIMER, QR, GAME };

// Draw icon centred at (cx,cy); r = half-size (icon fits in a 2r box).
void draw(TFT_eSPI& t, Id id, int16_t cx, int16_t cy, int16_t r, uint16_t c);
}  // namespace icon
