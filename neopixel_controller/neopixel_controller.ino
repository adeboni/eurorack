#include <Adafruit_NeoPixel.h>

#define RESET_LIMIT 100
#define LED_PIN     6
#define LED_COUNT   60

int p1 = A1;
int p1LL = 10000;
int p1UL = -1;
int p1RC = 0;
int p1V = 0;
bool p1Reset = false;

int p2 = A2;
int p2LL = 10000;
int p2UL = -1;
int p2RC = 0;
int p2V = 0;
bool p2Reset = false;

int p3 = A3;
int p3LL = 10000;
int p3UL = -1;
int p3RC = 0;
int p3V = 0;
bool p3Reset = false;

int rainbowWait = 15;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(64);
  strip.show();
}

void loop() {
  rainbowCycle();
}

void rainbowCycle() {
  for (uint16_t j=0; j<256*5; j++) {
    updateInput(p1, p1LL, p1UL, p1RC, p1V, p1Reset);
    updateInput(p2, p2LL, p2UL, p2RC, p2V, p2Reset);
    updateInput(p3, p3LL, p3UL, p3RC, p3V, p3Reset);
       
    int amp = map(p1V, 0, 100, LED_COUNT, 0);
    int bright = map(p2V, 0, 100, 7, 1);
    rainbowWait = map(p3V, 0, 100, 30, 2);
    
    for (uint16_t i = 0; i < amp; i++) {
      strip.setPixelColor(i, strip.Color(0,0,0));
    }
    for(uint16_t i = amp; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / LED_COUNT) + j) & 255, max(1, bright)));
    }
    strip.show();
    delay(rainbowWait);
  }
}

void updateInput(int pin, int &lowerLimit, int &upperLimit, int &resetCounter, int &lightValue, bool &hasReset) {
  int x = analogRead(pin);
  upperLimit = max(x, upperLimit);
  lowerLimit = min(x, lowerLimit);

  if (x < 10) {
    resetCounter++;
    if (resetCounter > RESET_LIMIT) {
       lowerLimit = x;
       upperLimit = x;
       resetCounter = 0;
       if (!hasReset) {
          hasReset = true;
          flashLights();
       }
    }
  }

  if (abs(upperLimit - lowerLimit) < 10) {
    if (x < 10)
      lightValue = 0;
    else
      lightValue = 100;
  }
  else {
    lightValue = map(x, lowerLimit, upperLimit, 0, 100);
    hasReset = false;
  }
}

void flashLights() {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0,0,0));
    }
    delay(500);
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(255,255,255));
    }
    delay(500);
}

uint32_t Wheel(byte WheelPos, int brightness) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color((255 - WheelPos * 3) / brightness, 0, (WheelPos * 3) / brightness);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, (WheelPos * 3) / brightness, (255 - WheelPos * 3) / brightness);
  } else {
   WheelPos -= 170;
   return strip.Color((WheelPos * 3) / brightness, (255 - WheelPos * 3) / brightness, 0);
  }
}
