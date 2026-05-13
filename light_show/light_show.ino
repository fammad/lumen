#include <Adafruit_NeoPixel.h>

#define LED_PIN    5
#define NUM_LEDS   16
#define BRIGHTNESS 200

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

enum State { CALM, ATTENTION, BREAK_STATE };
State currentState = CALM;
unsigned long lastStateChange = 0;
const unsigned long STATE_DURATION_MS = 20000;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Lumen state cycle test starting");
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  lastStateChange = millis();
}

void loop() {
  if (millis() - lastStateChange > STATE_DURATION_MS) {
    if (currentState == CALM)            currentState = ATTENTION;
    else if (currentState == ATTENTION)  currentState = BREAK_STATE;
    else                                 currentState = CALM;
    lastStateChange = millis();
    Serial.print("State -> ");
    Serial.println(currentState == CALM ? "CALM" :
                   currentState == ATTENTION ? "ATTENTION" : "BREAK");
  }

  uint16_t hue;
  uint8_t saturation;
  uint16_t cycleMs;
  uint8_t vMin, vMax;

  switch (currentState) {
    case CALM:
      hue = 6500; saturation = 90;     // warm white (orange hue, low saturation)
      cycleMs = 10000; vMin = 100; vMax = 255;
      break;
    case ATTENTION:
      hue = 5000; saturation = 255;    // amber (orange, full saturation)
      cycleMs = 5000; vMin = 100; vMax = 255;
      break;
    case BREAK_STATE:
      hue = 0; saturation = 255;       // pure red, dimmer range
      cycleMs = 5000; vMin = 130; vMax = 220;
      break;
  }

  // Triangle wave breath, 0..255
  uint16_t t = millis() % cycleMs;
  uint16_t halfCycle = cycleMs / 2;
  uint8_t wave;
  if (t < halfCycle) {
    wave = (t * 255) / halfCycle;
  } else {
    wave = ((cycleMs - t) * 255) / halfCycle;
  }

  // Map wave into vMin..vMax range
  uint8_t value = vMin + ((uint16_t)wave * (vMax - vMin)) / 255;

  // Same approach as the smooth rainbow: HSV + gamma correction
  uint32_t color = strip.gamma32(strip.ColorHSV(hue, saturation, value));
  strip.fill(color);
  strip.show();
  delay(20);
}