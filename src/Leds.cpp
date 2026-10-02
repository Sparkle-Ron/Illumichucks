#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"
#include "Leds.h"
namespace {
Adafruit_NeoPixel handle1(Config::HANDLE_1_COUNT, Config::HANDLE_1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel handle2(Config::HANDLE_2_COUNT, Config::HANDLE_2_PIN, NEO_GRB + NEO_KHZ800);
bool powered = false;
uint16_t frame = 0;
uint32_t lastFrameMs = 0;
bool preview = false;
uint32_t previewStartedMs = 0;
void dataLow() {
  pinMode(Config::HANDLE_1_PIN, OUTPUT); digitalWrite(Config::HANDLE_1_PIN, LOW);
  pinMode(Config::HANDLE_2_PIN, OUTPUT); digitalWrite(Config::HANDLE_2_PIN, LOW);
}
void fillBoth(uint32_t color) {
  handle1.fill(color); handle1.show();
  if (Config::ENABLE_HANDLE_2) { handle2.fill(color); handle2.show(); }
}
}
namespace Leds {
void begin() {
  digitalWrite(Config::LED_POWER_PIN, LOW);
  pinMode(Config::LED_POWER_PIN, OUTPUT);
  handle1.begin(); handle1.setBrightness(Config::BRIGHTNESS);
  if (Config::ENABLE_HANDLE_2) { handle2.begin(); handle2.setBrightness(Config::BRIGHTNESS); }
  dataLow();
}
void powerOn() {
  dataLow();
  digitalWrite(Config::LED_POWER_PIN, HIGH);
  delay(50); // Accommodates gate driver / future load switch and LED startup.
  powered = true;
  fillBoth(0);
  frame = 0; lastFrameMs = millis(); preview = false;
}
void powerOff() {
  if (powered) fillBoth(0);
  dataLow();
  digitalWrite(Config::LED_POWER_PIN, LOW);
  powered = false; preview = false;
}
void previewImpact(uint32_t now) {
  if (!powered) return;
  preview = true; previewStartedMs = now;
  fillBoth(handle1.Color(180, 0, 255));
}
void update(uint32_t now) {
  if (!powered) return;
  if (preview) {
    if (uint32_t(now - previewStartedMs) < Config::IMPACT_FLASH_MS) return;
    preview = false; fillBoth(0); lastFrameMs = now;
  }
  const uint32_t interval = frame < 6 ? Config::BLINK_INTERVAL_MS : Config::CHASE_INTERVAL_MS;
  if (uint32_t(now - lastFrameMs) < interval) return;
  lastFrameMs = now;
  const uint16_t chaseLength = Config::ENABLE_HANDLE_2 && Config::HANDLE_2_COUNT > Config::HANDLE_1_COUNT
                                ? Config::HANDLE_2_COUNT : Config::HANDLE_1_COUNT;
  if (frame < 6) {
    uint32_t color = 0;
    if (frame == 0) color = handle1.Color(255, 0, 0);
    if (frame == 2) color = handle1.Color(0, 255, 0);
    if (frame == 4) color = handle1.Color(0, 0, 255);
    fillBoth(color);
  } else {
    const uint16_t pixel = frame - 6;
    handle1.clear();
    if (pixel < Config::HANDLE_1_COUNT) handle1.setPixelColor(pixel, handle1.Color(255, 255, 255));
    handle1.show();
    if (Config::ENABLE_HANDLE_2) {
      handle2.clear();
      if (pixel < Config::HANDLE_2_COUNT) handle2.setPixelColor(pixel, handle2.Color(255, 255, 255));
      handle2.show();
    }
  }
  if (++frame >= 6 + chaseLength) frame = 0;
}
}
