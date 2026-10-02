#include <Arduino.h>
#include <mbed.h>
#include <nrf_gpio.h>
#include "Config.h"
#include "Motion.h"
namespace {
// These are PHYSICAL Nordic pins, not Arduino D-number indices.
// The onboard sensor has its own I2C bus, separate from external D4/D5.
// Construct the bus when setup calls begin(), after Arduino/core startup.
mbed::I2C& bus() {
  static mbed::I2C instance(P0_7, P0_27); // SDA, SCL
  return instance;
}
void logStage(const char* message) {
  if (Serial) { Serial.print("IMU: "); Serial.println(message); }
}
constexpr uint32_t IMU_INT_PIN = 11; // P0.11, onboard LSM6DS3 INT1
int address = 0x6A << 1; // mbed uses the 8-bit I2C address convention.
constexpr uint8_t WHO_AM_I=0x0F, CTRL1_XL=0x10, CTRL2_G=0x11, CTRL3_C=0x12;
constexpr uint8_t CTRL6_C=0x15, STATUS_REG=0x1E, OUTX_L_XL=0x28;
constexpr uint8_t WAKE_UP_SRC=0x1B, TAP_CFG=0x58, WAKE_UP_THS=0x5B;
constexpr uint8_t WAKE_UP_DUR=0x5C, MD1_CFG=0x5E, MD2_CFG=0x5F;
bool writeReg(uint8_t reg, uint8_t value) {
  const char bytes[] = {char(reg), char(value)};
  return bus().write(address, bytes, 2) == 0;
}
bool readRegs(uint8_t reg, uint8_t* values, int length) {
  const char index = char(reg);
  if (bus().write(address, &index, 1, true) != 0) return false;
  return bus().read(address, reinterpret_cast<char*>(values), length) == 0;
}
bool readReg(uint8_t reg, uint8_t& value) { return readRegs(reg, &value, 1); }
int16_t signedWord(uint8_t low, uint8_t high) {
  return static_cast<int16_t>((uint16_t(high) << 8) | low);
}
}
namespace Motion {
bool begin() {
  // P1.08 supplies the onboard IMU directly. Seeed's driver uses HIGH DRIVE
  // here; the earlier normal-drive DigitalOut can provide insufficient current.
  const uint32_t powerPin = NRF_GPIO_PIN_MAP(1, 8);
  nrf_gpio_pin_set(powerPin);
  nrf_gpio_cfg(powerPin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT,
               NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_H0H1, NRF_GPIO_PIN_NOSENSE);
  delay(50);
  bus().frequency(100000); // Conservative bring-up speed; ample for 208Hz samples.
  logStage("power HIGH DRIVE on P1.08; SDA=P0.07, SCL=P0.27");
  nrf_gpio_cfg_input(IMU_INT_PIN, NRF_GPIO_PIN_NOPULL);
  uint8_t id=0;
  bool found=false;
  for (int addr : {0x6A, 0x6B}) {
    address=addr << 1;
    const bool ok = readReg(WHO_AM_I,id);
    if (Serial) {
      Serial.print("IMU: address 0x"); Serial.print(addr, HEX);
      if (ok) { Serial.print(" WHO_AM_I=0x"); Serial.println(id, HEX); }
      else Serial.println(" did not respond to register read");
    }
    if (ok && (id==0x6A || id==0x69)) { found=true; break; }
  }
  if (!found) { logStage("no supported sensor found"); return false; }
  if (!writeReg(CTRL3_C,0x01)) { logStage("reset write failed"); return false; } // Software reset.
  const uint32_t start=millis();
  do {
    delay(1);
    if (!readReg(CTRL3_C,id)) { logStage("reset status read failed"); return false; }
    if (uint32_t(millis()-start)>100) { logStage("reset timed out"); return false; }
  } while (id & 0x01);
  // BDU + auto-increment; interrupts active HIGH, push-pull.
  if (!writeReg(CTRL3_C,0x44) || !writeReg(CTRL2_G,0x00) ||
      !writeReg(0x0D,0x00) || !writeReg(0x0E,0x00) || !writeReg(MD2_CFG,0x00)) {
    logStage("common register configuration failed"); return false;
  }
  const bool ready = configureActive();
  logStage(ready ? "initialization OK" : "active configuration failed");
  return ready;
}
bool configureActive() {
  nrf_gpio_cfg_input(IMU_INT_PIN, NRF_GPIO_PIN_NOPULL); // Remove sleep SENSE setting.
  // Active acceleration samples: +/-8g at 208Hz. Gyroscope remains OFF.
  if (!writeReg(MD1_CFG,0x00) || !writeReg(TAP_CFG,0x00) ||
      !writeReg(CTRL6_C,0x00) || !writeReg(CTRL1_XL,0x5C)) return false;
  uint8_t ignored=0;
  if (!readReg(WAKE_UP_SRC,ignored)) return false;
  delay(20); // Settle before motion baseline is initialized.
  return true;
}
ReadResult read(Sample& sample) {
  uint8_t status=0;
  if (!readReg(STATUS_REG,status)) return ReadResult::Error;
  if (!(status & 0x01)) return ReadResult::NotReady;
  uint8_t raw[6];
  if (!readRegs(OUTX_L_XL,raw,6)) return ReadResult::Error;
  constexpr float G_PER_LSB = 0.000244f; // +/-8g sensitivity: 0.244mg/LSB
  sample.x=signedWord(raw[0],raw[1])*G_PER_LSB;
  sample.y=signedWord(raw[2],raw[3])*G_PER_LSB;
  sample.z=signedWord(raw[4],raw[5])*G_PER_LSB;
  return ReadResult::Fresh;
}
bool prepareWake() {
  // Keep IMU powered in SYSTEM OFF; its low-power accelerometer watches motion.
  // +/-2g, 26Hz. Slope filter rejects static gravity; 1 count = 31.25mg.
  if (!writeReg(MD1_CFG,0x00) || !writeReg(CTRL2_G,0x00) ||
      !writeReg(CTRL6_C,0x10) || !writeReg(CTRL1_XL,0x20) ||
      !writeReg(TAP_CFG,0x81) || // Enable basic interrupts + latch; slope filter.
      !writeReg(WAKE_UP_THS,Config::WAKE_THRESHOLD_COUNTS) ||
      !writeReg(WAKE_UP_DUR,0x00)) return false;
  // ODR/range changes can cause an initial spurious event: allow >2 samples.
  delay(100);
  uint8_t ignored=0;
  if (!readReg(WAKE_UP_SRC,ignored) || !writeReg(MD1_CFG,0x20)) return false;
  // Prove key settings reached the device before committing to SYSTEM OFF.
  uint8_t route=0, threshold=0, control=0, tap=0;
  return readReg(MD1_CFG,route) && route==0x20 &&
         readReg(WAKE_UP_THS,threshold) && threshold==Config::WAKE_THRESHOLD_COUNTS &&
         readReg(CTRL1_XL,control) && control==0x20 &&
         readReg(TAP_CFG,tap) && tap==0x81;
}
bool wakePending() { return nrf_gpio_pin_read(IMU_INT_PIN) != 0; }
}
