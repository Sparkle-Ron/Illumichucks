#pragma once
#include <stdint.h>
namespace Leds {
void begin();
void powerOn();
void powerOff();
void update(uint32_t now);
void previewImpact(uint32_t now);
}
