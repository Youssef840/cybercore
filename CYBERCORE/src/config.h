#pragma once
// ============================================================================
//  CYBERCORE - global hardware & firmware configuration
//  Display pins live in platformio.ini (TFT_eSPI reads them as build flags).
//  PIN_NONE marks a feature that is not wired on the selected board.
// ============================================================================
#include <Arduino.h>

#define FW_NAME "CYBERCORE"
#define FW_VERSION "2.0.0"

constexpr uint8_t PIN_NONE = 255;

#if defined(BOARD_CYD)
// ===================== ESP32-2432S028R "Cheap Yellow Display" ===============
// CC1101 on the VSPI header lines
constexpr uint8_t PIN_CC_SCK = 18;
constexpr uint8_t PIN_CC_MISO = 19;
constexpr uint8_t PIN_CC_MOSI = 23;
constexpr uint8_t PIN_CC_CS = 27;
constexpr uint8_t PIN_CC_GDO0 = 22;
constexpr uint8_t PIN_CC_GDO2 = PIN_NONE;
constexpr uint8_t PIN_SD_CS = 5;  // on-board microSD shares VSPI: keep deselected

// Only the BOOT button exists; the rest comes from the touchscreen
constexpr uint8_t PIN_BTN_UP = PIN_NONE;
constexpr uint8_t PIN_BTN_DOWN = PIN_NONE;
constexpr uint8_t PIN_BTN_OK = PIN_NONE;
constexpr uint8_t PIN_BTN_BACK = 0;

// XPT2046 resistive touch (own pins, bit-banged -> no SPI bus sharing)
#define HAS_TOUCH 1
constexpr uint8_t PIN_TOUCH_CLK = 25;
constexpr uint8_t PIN_TOUCH_MOSI = 32;
constexpr uint8_t PIN_TOUCH_MISO = 39;
constexpr uint8_t PIN_TOUCH_CS = 33;
constexpr uint8_t PIN_TOUCH_IRQ = 36;
// Raw ADC -> screen mapping (verify with the serial "touch" command)
constexpr int TOUCH_X_MIN = 200, TOUCH_X_MAX = 3700;  // raw from cmd 0xD0
constexpr int TOUCH_Y_MIN = 240, TOUCH_Y_MAX = 3800;  // raw from cmd 0x90
constexpr bool TOUCH_FLIP_X = true;                   // rotation 3 = flipped
constexpr bool TOUCH_FLIP_Y = true;

// On-board RGB LED (active LOW)
constexpr uint8_t PIN_LED_R = 4, PIN_LED_G = 16, PIN_LED_B = 17;

#else
// ===================== Generic ESP32 DevKit + wired TFT =====================
constexpr uint8_t PIN_CC_SCK = 18;
constexpr uint8_t PIN_CC_MISO = 19;
constexpr uint8_t PIN_CC_MOSI = 23;
constexpr uint8_t PIN_CC_CS = 5;
constexpr uint8_t PIN_CC_GDO0 = 26;
constexpr uint8_t PIN_CC_GDO2 = 25;
constexpr uint8_t PIN_SD_CS = PIN_NONE;

constexpr uint8_t PIN_BTN_UP = 21;
constexpr uint8_t PIN_BTN_DOWN = 22;
constexpr uint8_t PIN_BTN_OK = 4;
constexpr uint8_t PIN_BTN_BACK = 0;  // on-board BOOT button

constexpr uint8_t PIN_LED_R = PIN_NONE, PIN_LED_G = PIN_NONE, PIN_LED_B = PIN_NONE;
#endif

#ifndef DISPLAY_ROTATION
#define DISPLAY_ROTATION 1
#endif

// ---- Timing -----------------------------------------------------------------
constexpr uint32_t BTN_DEBOUNCE_MS = 25;
constexpr uint32_t BTN_LONG_MS = 700;
constexpr uint32_t BTN_REPEAT_DLY = 400;
constexpr uint32_t BTN_REPEAT_MS = 90;

// ---- RF defaults ------------------------------------------------------------
constexpr int16_t RSSI_FLOOR_DBM = -120;
constexpr int16_t RSSI_CEIL_DBM = -20;
