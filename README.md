# Illumichuck — PlatformIO / VS Code

Open this folder (the one containing platformio.ini) using PlatformIO Home > Open Project.
The configuration uses Seeed's mbed platform and the XIAO nRF52840 Sense board.
Do not substitute the Adafruit/non-mbed XIAO board: this firmware uses mbed::I2C.
The platform revision and NeoPixel version are pinned for reproducible builds.
PlatformIO manages its own core and libraries; your Arduino IDE installations are separate.

## First run

1. Extract the ZIP and open the Illumichuck_PlatformIO folder in VS Code.
2. Allow PlatformIO to install the platform, compiler, and library. First setup takes longer.
3. Connect the XIAO with a USB data cable.
4. Click the PlatformIO Build checkmark, then Upload arrow in the bottom toolbar.
   These tasks are also under PlatformIO > Project Tasks > xiao_nrf52840_sense.
5. Open Monitor (115200 baud). Press reset to see the startup log.

If automatic port selection fails, select your board's port in PlatformIO or set
upload_port and monitor_port in platformio.ini to your actual port.
If uploading cannot find the board, double-press reset to enter its bootloader,
then retry Upload. The bootloader port may differ from the normal serial port.
Close other serial monitors before uploading if the port is busy.

## Files and settings

- src/main.cpp: application state, inactivity timer, sleep/wake, impact event hook.
- include/Config.h: pins, LED counts/brightness, wake threshold, timings.
- src/Leds.cpp: animation and LED power sequencing.
- src/Motion.cpp: onboard IMU and movement interrupt, including the working
  P1.08 high-drive power fix.
- src/MotionLogic.cpp: active movement/impact detection.
- src/Power.cpp: battery SYSTEM OFF and USB standby.
- include/: corresponding module headers.

Hardware: 30 LEDs per handle, 60 total; handle 1 data D9; handle 2 data D5;
LED power control D10 HIGH=ON through the BJT/P-MOSFET circuit; brightness 16.

WAKE_THRESHOLD_COUNTS starts at 1, matching the provided firmware. Copy your
latest tuned value here if you changed it in Arduino IDE. Each count represents
0.03125g of filtered motion in the sleep configuration. Raise to 2 or 3 and test.
ACTIVE_MOTION_THRESHOLD_G separately controls movement while already awake.

After each source/config edit, save and Upload to update the board. Build only
compiles; it does not program the board. Battery operation uses SYSTEM OFF after
3 seconds of inactivity; movement resets the processor and runs setup again.
USB operation uses lighter standby to retain programming/serial access.
Startup waits up to 5 seconds for Serial only when USB is connected.

BLE remains a future module. This migration does not implement phone control.

## Validation

The migrated source compiles with Seeed mbed core 2.9.3 and NeoPixel 1.15.5
using Arduino CLI (94,168 bytes flash, 44,440 bytes RAM). A complete PlatformIO
build could not be verified here because package-mirror downloads failed.
PlatformIO uses Seeed's packaged mbed core 2.9.2, so please run Build before Upload.
No hardware upload was performed during migration.
