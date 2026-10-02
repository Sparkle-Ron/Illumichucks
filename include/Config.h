#pragma once
#include <Arduino.h>

namespace Config {
constexpr uint8_t HANDLE_1_PIN = D9;
constexpr uint8_t HANDLE_2_PIN = D5;
constexpr uint8_t LED_POWER_PIN = D10; // HIGH = ON through BJT or load-switch IC
constexpr uint16_t HANDLE_1_COUNT = 30;
constexpr uint16_t HANDLE_2_COUNT = 30;
constexpr bool ENABLE_HANDLE_2 = true;
constexpr uint8_t BRIGHTNESS = 16;
constexpr uint32_t IDLE_TIMEOUT_MS = 3000; // Three seconds since last movement
constexpr uint32_t BLINK_INTERVAL_MS = 500;
constexpr uint32_t CHASE_INTERVAL_MS = 100;
// True: SYSTEM OFF on battery. USB automatically uses lighter standby.
// False: use lighter standby everywhere while diagnosing motion wake.
constexpr bool ENABLE_SYSTEM_OFF = true;
// Sleep range is +/-2g: each count = 2/64g = 0.03125g of filtered motion.
constexpr uint8_t WAKE_THRESHOLD_COUNTS = 5; // 1..63; raise if too sensitive
constexpr float ACTIVE_MOTION_THRESHOLD_G = 0.06f;
// This is a first-pass impact detector, not yet a reliable body-slap classifier.
constexpr float IMPACT_THRESHOLD_G = 4.0f; // Total acceleration; rest is about 1g
constexpr uint32_t IMPACT_COOLDOWN_MS = 400;
constexpr bool ENABLE_IMPACT_PREVIEW = false; // true = brief purple flash
constexpr uint32_t IMPACT_FLASH_MS = 200;
constexpr bool LOG_MOTION_TELEMETRY = false; // USB log every 500ms when enabled
}
static_assert(Config::WAKE_THRESHOLD_COUNTS >= 1 && Config::WAKE_THRESHOLD_COUNTS <= 63,
              "Wake threshold must be 1..63");
