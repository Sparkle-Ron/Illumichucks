/* Illumichuck: 60 LEDs, inactivity sleep and accelerometer wake.
 * Board: Seeed XIAO nRF52840 Sense under Seeed nRF52 mbed-enabled Boards.
 * Library: Adafruit NeoPixel. No BLE or external IMU library needed yet.
 * PlatformIO: open the folder containing platformio.ini in VS Code.
 */
#include <Arduino.h>
#include "Config.h"
#include "Leds.h"
#include "Motion.h"
#include "MotionLogic.h"
#include "Power.h"

namespace {
enum class State { Active, Standby, Fault };
State state = State::Fault;
MotionLogic motionLogic(Config::ACTIVE_MOTION_THRESHOLD_G,
                        Config::IMPACT_THRESHOLD_G,
                        Config::IMPACT_COOLDOWN_MS);
uint32_t lastMovementMs = 0;
uint32_t lastTelemetryMs = 0;
uint8_t readErrors = 0;

void fail(const char* message) {
  Leds::powerOff();
  state = State::Fault;
  if (Power::usbPresent()) Serial.println(message);
}

void reportAwake() {
  if (Power::usbPresent()) Serial.println("ACTIVE: move to keep LEDs on; rest 3s to sleep.");
}

void wakeFromStandby() {
  if (!Motion::configureActive()) {
    fail("ERROR: IMU resume failed. LEDs off; reset to retry.");
    return;
  }
  motionLogic.reset();
  readErrors = 0;
  lastMovementMs = millis();
  Leds::powerOn();
  state = State::Active;
  reportAwake();
}

void goToSleep() {
  // Blank while powered, then drive BOTH data pins low, THEN remove LED power.
  Leds::powerOff();
  if (!Motion::prepareWake()) {
    fail("ERROR: wake setup failed. LEDs off; reset to retry.");
    return;
  }
  state = State::Standby;
  if (Power::usbPresent()) Serial.println("STANDBY: LED power off; move to wake.");
  // A pending motion event or a newly connected USB cable cancels SYSTEM OFF.
  // In real SYSTEM OFF, movement resets the MCU and runs setup() again.
  if (Config::ENABLE_SYSTEM_OFF && !Power::usbPresent()) Power::trySystemOff();
}
}

void setup() {
  Power::begin();
  Leds::begin(); // Initializes data pins and keeps LED power OFF.
  Serial.begin(115200);
  // Capture startup diagnostics on USB; battery startup has no serial wait.
  if (Power::usbPresent()) {
    const uint32_t serialStart = millis();
    while (!Serial && uint32_t(millis() - serialStart) < 5000) delay(10);
    Serial.println("Starting Illumichuck...");
  }
  if (!Motion::begin()) {
    fail("ERROR: onboard IMU not found/configured. LEDs off; reset to retry.");
    return;
  }
  lastMovementMs = millis();
  Leds::powerOn();
  state = State::Active;
  reportAwake();
}

void loop() {
  if (state == State::Fault) { delay(100); return; }
  if (state == State::Standby) {
    if (Motion::wakePending()) {
      // configureActive clears the latched wake interrupt.
      wakeFromStandby();
    } else if (Config::ENABLE_SYSTEM_OFF && !Power::usbPresent()) {
      // USB may have been unplugged since entering standby.
      Power::trySystemOff();
    }
    delay(20); // RTOS yields in USB/debug standby; no LED animation runs.
    return;
  }

  const uint32_t now = millis();
  Motion::Sample sample;
  const Motion::ReadResult result = Motion::read(sample);
  if (result == Motion::ReadResult::Error) {
    if (++readErrors >= 3) fail("ERROR: IMU read failed. LEDs off; reset to retry.");
  } else if (result == Motion::ReadResult::Fresh) {
    readErrors = 0;
    const MotionLogic::Event event = motionLogic.update(sample.x, sample.y, sample.z, now);
    if (event.moving) lastMovementMs = now;
    if (event.impact) {
      lastMovementMs = now;
      // Future BLE/settings/effect selection can hook into this event.
      if (Power::usbPresent()) {
        Serial.print("IMPACT: "); Serial.print(event.totalG, 2); Serial.println(" g");
      }
      if (Config::ENABLE_IMPACT_PREVIEW) Leds::previewImpact(now);
    }
    if (Config::LOG_MOTION_TELEMETRY && Power::usbPresent() &&
        uint32_t(now - lastTelemetryMs) >= 500) {
      lastTelemetryMs = now;
      Serial.print("accel="); Serial.print(event.totalG, 2);
      Serial.print("g motion="); Serial.print(event.motionG, 3);
      Serial.print("g idle="); Serial.println(uint32_t(now - lastMovementMs));
    }
  }
  if (state != State::Active) return;
  if (MotionLogic::idleExpired(now, lastMovementMs, Config::IDLE_TIMEOUT_MS)) {
    goToSleep();
    return;
  }
  Leds::update(now);
  delay(2); // Sensor runs at 208Hz; process new samples without busy spinning.
}
