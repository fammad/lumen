/*
  lumen_firmware_step2.ino

  Adds tap/hold gestures and the PAUSED + BREATHING states.

  Behavior:
    - Starts in CALM. Ambient breathing animation runs.
    - Tap (< 500 ms):  toggle PAUSED. Dim steady white. Tap again → resume.
    - Hold (>= 1000 ms): start BREATHING exercise (4-7-8, soft green, 8 cycles).
                         Hold again during exercise → cancel on next cycle.
                         Exercise auto-completes after 8 cycles, returns to ambient.

  Pre-Step 3 (no serial state changes yet):
    - State starts in CALM and stays in CALM unless gestures activate PAUSED/BREATHING.
    - To test ATTENTION or BREAK colors, change `currentState = CALM` initialization
      at the top, re-flash, observe. We'll wire Python → serial → state in Step 3.

  Replaces breathing math placeholder (cosine) once Yuting picks a variation.

  Hardware (per 2026-05-09 session):
    NeoPixel ring DI → GPIO 5
    Button         → GPIO 4 (INPUT_PULLUP, other leg to GND)
    Brightness     = 150
    Color order    = NEO_GRB
*/

#include <Adafruit_NeoPixel.h>

#define LED_PIN     5
#define BUTTON_PIN  4
#define NUM_LEDS    16

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);


// ============================================================
// STATE
// ============================================================

// "BREAK_STATE" not "BREAK" because `break` is a C++ keyword.
enum State { CALM, ATTENTION, BREAK_STATE, PAUSED, BREATHING };

State currentState     = CALM;
State prevAmbientState = CALM;   // remembered when entering PAUSED/BREATHING


// ============================================================
// COLOR & TIMING — locked values from 2026-05-09 session
// ============================================================

// CALM
const uint16_t      CALM_HUE       = 6500;
const uint8_t       CALM_SAT       = 90;
const unsigned long CALM_CYCLE_MS  = 10000;
const uint8_t       CALM_VMIN      = 100;
const uint8_t       CALM_VMAX      = 255;

// ATTENTION
const uint16_t      ATTN_HUE       = 5000;
const uint8_t       ATTN_SAT       = 255;
const unsigned long ATTN_CYCLE_MS  = 5000;
const uint8_t       ATTN_VMIN      = 100;
const uint8_t       ATTN_VMAX      = 255;

// BREAK
const uint16_t      BRK_HUE        = 0;
const uint8_t       BRK_SAT        = 255;
const unsigned long BRK_CYCLE_MS   = 5000;
const uint8_t       BRK_VMIN       = 130;
const uint8_t       BRK_VMAX       = 220;

// PAUSED — dim steady white. Saturation 0 = pure white (hue ignored).
const uint8_t       PAUSED_VALUE   = 30;

// BREATHING — 4-7-8 cycle, soft green
const uint16_t      BRTH_HUE       = 22000;    // green
const uint8_t       BRTH_SAT       = 200;
const uint8_t       BRTH_VMIN      = 30;
const uint8_t       BRTH_VMAX      = 255;
const unsigned long INHALE_MS      = 4000;
const unsigned long BHOLD_MS       = 7000;
const unsigned long EXHALE_MS      = 8000;
const int           BREATHING_TARGET_CYCLES = 8;


// ============================================================
// TAP / HOLD DETECTION
// ============================================================

const unsigned long TAP_MAX_MS   = 500;
const unsigned long HOLD_MIN_MS  = 1000;
const unsigned long DEBOUNCE_MS  = 50;

bool          touchActive    = false;
unsigned long touchStartMs   = 0;
bool          holdFired      = false;
bool          lastButtonRead = HIGH;
unsigned long lastDebounceMs = 0;


// ============================================================
// BREATHING EXERCISE STATE
// ============================================================

int           breathingCycleCount   = 0;
unsigned long breathingCycleStartMs = 0;


// ============================================================
// SETUP & LOOP
// ============================================================

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  strip.begin();
  strip.setBrightness(150);
  strip.show();

  Serial.println();
  Serial.println("Lumen firmware Step 2");
  Serial.println("  Tap  (<500ms):   toggle PAUSED");
  Serial.println("  Hold (>=1000ms): toggle BREATHING (4-7-8)");
  Serial.print  ("  Initial state:   ");
  printState(currentState);
}

void loop() {
  handleButton();
  renderState();
  delay(20);   // ~50 fps
}


// ============================================================
// BUTTON HANDLING — non-blocking tap/hold detection
// ============================================================

void handleButton() {
  bool reading      = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  // Debounce: if reading changed since last frame, restart debounce timer.
  if (reading != lastButtonRead) {
    lastDebounceMs = now;
  }
  lastButtonRead = reading;

  // Ignore frames until the reading has been stable for DEBOUNCE_MS.
  if ((now - lastDebounceMs) < DEBOUNCE_MS) {
    return;
  }

  // Stable reading. INPUT_PULLUP: pressed = LOW.
  bool pressed = (reading == LOW);

  if (pressed && !touchActive) {
    // JUST PRESSED — start measuring duration
    touchActive  = true;
    touchStartMs = now;
    holdFired    = false;
  }
  else if (pressed && touchActive && !holdFired) {
    // STILL HELD — has duration crossed the hold threshold?
    if ((now - touchStartMs) >= HOLD_MIN_MS) {
      toggleBreathing();
      holdFired = true;        // prevent re-firing until release
    }
  }
  else if (!pressed && touchActive) {
    // JUST RELEASED — was it a tap?
    unsigned long duration = now - touchStartMs;
    if (!holdFired && duration < TAP_MAX_MS) {
      togglePaused();
    }
    // 500ms ≤ duration < 1000ms = dead zone, no action
    touchActive = false;
  }
}


// ============================================================
// STATE TRANSITIONS
// ============================================================

void togglePaused() {
  if (currentState == PAUSED) {
    currentState = prevAmbientState;
    Serial.print("Tap: resumed → ");
    printState(currentState);
  } else if (currentState == BREATHING) {
    // Don't pause during a breathing exercise — let the user complete or hold-cancel
    Serial.println("Tap ignored (in BREATHING)");
  } else {
    prevAmbientState = currentState;
    currentState     = PAUSED;
    Serial.println("Tap: paused");
  }
}

void toggleBreathing() {
  if (currentState == BREATHING) {
    // Cancel on next cycle boundary by jumping the count to target
    breathingCycleCount = BREATHING_TARGET_CYCLES;
    Serial.println("Hold: breathing cancelled (ends on next cycle)");
  } else {
    // Remember where to return. If currently paused, return to CALM by default.
    prevAmbientState     = (currentState == PAUSED) ? CALM : currentState;
    currentState         = BREATHING;
    breathingCycleCount  = 0;
    breathingCycleStartMs = millis();
    Serial.println("Hold: breathing started (4-7-8 × 8)");
  }
}


// ============================================================
// RENDERING
// ============================================================

void renderState() {
  unsigned long now = millis();

  switch (currentState) {
    case CALM:
      renderBreathing(now, CALM_HUE, CALM_SAT, CALM_VMIN, CALM_VMAX, CALM_CYCLE_MS);
      break;
    case ATTENTION:
      renderBreathing(now, ATTN_HUE, ATTN_SAT, ATTN_VMIN, ATTN_VMAX, ATTN_CYCLE_MS);
      break;
    case BREAK_STATE:
      renderBreathing(now, BRK_HUE, BRK_SAT, BRK_VMIN, BRK_VMAX, BRK_CYCLE_MS);
      break;
    case PAUSED:
      renderPausedWhite();
      break;
    case BREATHING:
      renderBreathingExercise(now);
      break;
  }
}

// Cosine breathing — placeholder until Yuting's variation is locked.
void renderBreathing(unsigned long now,
                     uint16_t hue, uint8_t sat,
                     uint8_t vMin, uint8_t vMax,
                     unsigned long cycleMs) {
  float t = (now % cycleMs) / (float)cycleMs;
  float brightness = 0.5 - 0.5 * cos(t * 2.0 * PI);
  uint8_t value = vMin + (uint8_t)((vMax - vMin) * brightness);
  uint32_t color = strip.gamma32(strip.ColorHSV(hue, sat, value));
  strip.fill(color);
  strip.show();
}

void renderPausedWhite() {
  // Saturation = 0 produces pure white regardless of hue
  uint32_t color = strip.gamma32(strip.ColorHSV(0, 0, PAUSED_VALUE));
  strip.fill(color);
  strip.show();
}

void renderBreathingExercise(unsigned long now) {
  unsigned long elapsed    = now - breathingCycleStartMs;
  unsigned long totalCycle = INHALE_MS + BHOLD_MS + EXHALE_MS;
  float brightness;

  if (elapsed < INHALE_MS) {
    // Phase 1: linear rise 0 → 1 over 4 seconds
    brightness = (float)elapsed / INHALE_MS;
  }
  else if (elapsed < INHALE_MS + BHOLD_MS) {
    // Phase 2: hold at 1 for 7 seconds
    brightness = 1.0;
  }
  else if (elapsed < totalCycle) {
    // Phase 3: linear fall 1 → 0 over 8 seconds
    unsigned long exhaleElapsed = elapsed - INHALE_MS - BHOLD_MS;
    brightness = 1.0 - ((float)exhaleElapsed / EXHALE_MS);
  }
  else {
    // Cycle complete — increment count and reset
    breathingCycleCount++;
    breathingCycleStartMs = now;

    if (breathingCycleCount >= BREATHING_TARGET_CYCLES) {
      // Done — return to ambient
      currentState = prevAmbientState;
      Serial.print("Breathing complete → ");
      printState(currentState);
      return;
    }
    brightness = 0.0;
  }

  uint8_t value = BRTH_VMIN + (uint8_t)((BRTH_VMAX - BRTH_VMIN) * brightness);
  uint32_t color = strip.gamma32(strip.ColorHSV(BRTH_HUE, BRTH_SAT, value));
  strip.fill(color);
  strip.show();
}


// ============================================================
// DEBUG
// ============================================================

void printState(State s) {
  switch (s) {
    case CALM:        Serial.println("CALM");        break;
    case ATTENTION:   Serial.println("ATTENTION");   break;
    case BREAK_STATE: Serial.println("BREAK");       break;
    case PAUSED:      Serial.println("PAUSED");      break;
    case BREATHING:   Serial.println("BREATHING");   break;
  }
}
