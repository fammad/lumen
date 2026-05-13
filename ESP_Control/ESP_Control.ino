#include <Adafruit_NeoPixel.h>

#define LED_PIN    5
#define NUM_LEDS   16
#define BRIGHTNESS 200   // Pushing close to USB limit. If board disconnects, drop to 180.

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.print("Whole-ring hue cycle, brightness=");
  Serial.println(BRIGHTNESS);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void loop() {
  static uint16_t hue = 0;
  uint32_t color = strip.gamma32(strip.ColorHSV(hue));
  strip.fill(color);
  strip.show();
  hue += 64;       // Smaller = slower transition
  delay(20);       // Larger = slower transition
}