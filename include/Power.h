#pragma once
namespace Power {
void begin();
bool usbPresent();
// Returns only if sleep was cancelled or a debugger emulates SYSTEM OFF.
void trySystemOff();
}
