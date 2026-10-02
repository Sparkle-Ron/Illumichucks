#include <Arduino.h>
#include <nrf.h>
#include <nrf_gpio.h>
#include "Power.h"
#include "Motion.h"
namespace Power {
void begin() {
  // Disable SENSE inherited from a previous SYSTEM OFF wake.
  nrf_gpio_cfg_input(11, NRF_GPIO_PIN_NOPULL);
  // RGB LEDs are active LOW; ensure they're not a standby drain.
  for (uint32_t pin : {26u, 30u, 6u}) {
    nrf_gpio_pin_set(pin); nrf_gpio_cfg_output(pin);
  }
  // Onboard microphone power off (not used by this project).
  nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(1,10));
  nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(1,10));
  // Leave battery divider enabled LOW per Seeed's charging guidance.
  nrf_gpio_pin_clear(14); nrf_gpio_cfg_output(14);
}
bool usbPresent() {
  return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
}
void trySystemOff() {
  if (usbPresent() || Motion::wakePending()) return;
  // Physical P0.11 is the onboard IMU interrupt; not Arduino pin 11 or D11.
  nrf_gpio_cfg_sense_input(11, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
  if (usbPresent() || Motion::wakePending()) {
    nrf_gpio_cfg_input(11, NRF_GPIO_PIN_NOPULL);
    return;
  }
  __DSB();
  NRF_POWER->SYSTEMOFF = 1;
  __DSB(); __ISB();
  // Hardware stays off until GPIO DETECT or reset. With a debugger attached,
  // SYSTEM OFF can be emulated; let the app return to standby in that case.
  __WFE();
}
}
