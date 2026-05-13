# Lumen Hardware Session Notes — 2026-05-09

## Hardware change from spec v7
Spec v7 §5.2 specified XIAO ESP32-S3. Actual hardware: Freenove ESP32-S3-WROOM dev board with GPIO Extension Board on breadboard. Camera module detached. Same chip family — firmware logic ports directly.

## Locked Arduino IDE settings (REVISED — switched away from OTG)
- Board: ESP32S3 Dev Module
- **USB port used: UART port** (NOT the OTG port — Serial Monitor was unreliable on OTG due to USB CDC re-enumeration timing)
- **USB Mode: Hardware CDC and JTAG** (changed from earlier "USB-OTG CDC (TinyUSB)")
- Port appears as `/dev/cu.usbmodem101` when using UART port
- Upload Mode: UART0 / Hardware CDC
- USB CDC On Boot: Enabled
- CPU Frequency: 240MHz
- Flash Size: 4MB
- Partition Scheme: Default 4MB with spiffs
- Upload Speed: 921600
- ESP32 board package version: 3.0.7

## Verified working
- ✅ Smoke test: clean Serial output, "tick" prints reliably
- ✅ NeoPixel ring: 16 LEDs respond, direct GPIO 5 drive (no level shifter needed)
- ✅ Color order confirmed as **NEO_GRB** (NEO_RGB tested and produced wrong colors)
- ✅ Button: GPIO 4 + GND with INPUT_PULLUP, debounced reads working
- ✅ State machine cycle test: CALM (warm white) → ATTENTION (amber) → BREAK (red) breathing correctly
- ✅ Smooth breathing achieved using `ColorHSV()` + `gamma32()` (NOT raw RGB math — that produced color flips at low brightness)

## Wiring (current breadboard layout)
- Ring 5V → 5V rail (powered via jumper from extension board "5V" pin)
- Ring GND → GND rail (jumper from extension board "GND" pin)
- Ring DI → GPIO 5 (extension pin "5")
- Button: one leg → GPIO 4, other leg (opposite side, NOT same-side pair) → GND rail
- Brightness: 150 confirmed safe on USB
- No 74HCT125 level shifter, no 1000µF cap, no inline resistor — direct drive works

## State color/timing values (locked working version)
| State | Hue | Saturation | Cycle (ms) | vMin | vMax |
|---|---|---|---|---|---|
| CALM | 6500 | 90 | 10000 | 100 | 255 |
| ATTENTION | 5000 | 255 | 5000 | 100 | 255 |
| BREAK | 0 | 255 | 5000 | 130 | 220 |

Renders via: `strip.gamma32(strip.ColorHSV(hue, saturation, value))`

Note: BREAK uses higher vMin floor (130, ~50%) than spec v7's 10–60% amplitude. Reason: WS2812B color shifts unreliably at low PWM duty. Trade-off accepted for stability. Revisit after dome arrives — translucent PLA may mask shifts and allow lower floor.

## Active issue at end of session
**Marginal solder joints on ring's wire pads.** Hands-off the ring runs cleanly through full state cycle. Touching the ring triggers flicker. Joints make contact at rest but mechanical stress (handling, vibration) disrupts the data signal.

Diagnosed via 60-second hands-off test: smooth breathing, no flicker, all transitions clean.

**Next action:** Reflow all three solder joints (5V, GND, DI), then mechanical wiggle-test. Replace ring only if reflow fails or pad lifts. Backup ring available.

## Where firmware build stands (Option B incremental)
- ✅ Step 1 of 3: state machine + breathing animations (auto-cycle, no button)
- ⏳ Step 2: tap/hold detection on button (PAUSED state on tap, BREATHING state on hold)
- ⏳ Step 3: bidirectional Serial commands (P/R/X/E events between Python and ESP32)

## Pending after firmware steps
- T3.4: Setup description document (deadline May 14)
- T3.3: Python `Lumen` class implementation (spec v7 §5.4)
- T3.5: End-to-end integration test (webcam → risk score → ESP32 → LED)
- Dome arrival from KTH Middla
- T4.3: Conference lighting test
- Sprint 4: video, abstract finalization, submit by May 16