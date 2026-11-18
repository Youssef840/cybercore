// ============================================================================
//  CYBERCORE Firmware - entry point
// ============================================================================
#include <Arduino.h>
#include "core/App.h"

void setup() { App::instance().begin(); }
void loop() { App::instance().loop(); }
