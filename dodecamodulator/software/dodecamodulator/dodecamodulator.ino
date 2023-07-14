#include <EEPROM.h>
#include <Adafruit_MCP4728.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Encoder.h>
#include <Bounce2.h>
#include "Basics.h"

#define SELECT_BTN      33
#define DOWN_BTN        34
#define UP_BTN          35

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

#define MAX_OUTPUT      2048
#define HALF_MAX_OUTPUT 1024
#define UPDATE_RATE_US  2000

Adafruit_MCP4728 mcp[3];

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Encoder encoder(UP_BTN, DOWN_BTN);
Bounce button = Bounce();
int selectionMode = -1;

int dacMapping[] = { 2, 1, 0, 0, 2, 2, 1, 0, 2, 1, 1, 0 };
MCP4728_channel_t channelMapping[] = { 
  MCP4728_CHANNEL_D, MCP4728_CHANNEL_D, MCP4728_CHANNEL_D, MCP4728_CHANNEL_A, 
  MCP4728_CHANNEL_C, MCP4728_CHANNEL_A, MCP4728_CHANNEL_A, MCP4728_CHANNEL_B,
  MCP4728_CHANNEL_B, MCP4728_CHANNEL_C, MCP4728_CHANNEL_B, MCP4728_CHANNEL_C 
};

float rates[] = { 0.3, 1, 6, 12, 0.3, 1, 6, 12, 0.3, 1, 6, 12 };
int periods[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int squareLen[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int squareDir[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int triangleDir[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int triangleVal[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
float sinAngle[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int outputs[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void pcaselect(const byte channel) {
  Wire.beginTransmission(0x70);
  Wire.write(0x04 | channel);
  Wire.endTransmission();
}

void saveRates() {
   EEPROM.begin(64);
   int address = 0;
   int dataSaved = 1;
   EEPROM.write(address, dataSaved);
   address += sizeof(dataSaved);
   for (int i = 0; i < 12; i++) {
     EEPROM.writeFloat(address, rates[i]);
     address += sizeof(rates[i]);
   }
   EEPROM.commit();
   EEPROM.end();

   for (int i = 0; i < 12; i++)
     periods[i] = 1000000 / ((int)(UPDATE_RATE_US * rates[i]));
}

void loadRates() {
  EEPROM.begin(64);
   int address = 0;
   int dataSaved = EEPROM.read(address);
   address += sizeof(dataSaved);
   if (dataSaved == 1) {
     for (int i = 0; i < 12; i++) {
       rates[i] = EEPROM.readFloat(address);
       address += sizeof(rates[i]);
     }
   }
   EEPROM.end();
}

void displayMessage(const char* msg) {
  pcaselect(3);
  display.clearDisplay();
  display.setCursor(5, 5);
  display.print(msg);
  display.display();
}

bool isRateSelected(const int rateIndex) {
  if      (selectionMode == 0) return true;
  else if (selectionMode <= 3) return (rateIndex / 4) == (selectionMode - 1);
  else if (selectionMode <= 7) return (rateIndex % 4) == (selectionMode - 4);
  else                         return rateIndex == (selectionMode - 8);
}

void displayRates() {
  pcaselect(3);
  display.clearDisplay();
  for (int i = 0; i < 12; i++) {
    display.setCursor((i % 4) * 30 + 10, (i / 4) * 20 + 9);
    char buf[16];
    if (rates[i] < 3)
      sprintf(buf, "%.1f", rates[i]);
    else
      sprintf(buf, "%d", (int)rates[i]);

    if (isRateSelected(i))
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print(buf);
    display.setTextColor(SH110X_WHITE);
  }
  display.display();
}

void setup() {
  loadRates();
  saveRates();
  button.attach(SELECT_BTN, INPUT_PULLUP);
  button.interval(5);
  Wire.begin();
  Wire.setClock(400000);

  pcaselect(3);
  display.begin(0x3C, true);
  display.setRotation(2);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.display();
  displayRates();
  
  for (int i = 0; i < 3; i++) {
    pcaselect(i);
    if (!mcp[i].begin()) {
      displayMessage("DAC Failure");
      while (1);
    }
  }
}

void loop() {
  updateUI();
  updateVals();  
}

void updateUI() {
  static int pos = 0;
  int dir = 0;
  button.update();
  int newPos = encoder.read() / 4;
  if (pos != newPos) {
    dir = newPos > pos ? 1 : -1;
    pos = newPos;
  }
  if (dir != 0)  {
    if (button.read() == LOW) {
      for (int i = 0; i < 12; i++) {
        if (isRateSelected(i)) {
          if (rates[i] < 3) rates[i] = constrain(rates[i] + 0.1 * dir, 0.1, 20);
          else rates[i] = constrain(rates[i] + dir, 0.1, 20);
        }
      }
      saveRates();
    } else {
      selectionMode = constrain(selectionMode + dir, -1, 20);
    }
    
    displayRates();
  }
}

void updateVals() {
  static unsigned long lastTime = 0;
  unsigned long newTime = micros();
  if (newTime - lastTime < UPDATE_RATE_US)
    return;
  lastTime = newTime;
    
  for (int i = 0; i < 12; i++) { 
    if (i >> 2 == 0) {
      if (++squareLen[i] > (periods[i] >> 1)) {
        squareLen[i] = 0;
        squareDir[i] *= -1;
      }
      outputs[i] = squareDir[i] > 0 ? MAX_OUTPUT : 0;
    } else if (i >> 2 == 1) {
      triangleVal[i] += (2 * MAX_OUTPUT * triangleDir[i]) / periods[i];
      outputs[i] = triangleVal[i];
      if (outputs[i] >= MAX_OUTPUT) {
        triangleDir[i] = -1;
        outputs[i] = MAX_OUTPUT;
      } else if (outputs[i] <= 0) {
        triangleDir[i] = 1;
        outputs[i] = 0;
      }
    } else {
      sinAngle[i] += 360.0 / periods[i];
      if (sinAngle[i] > 360) sinAngle[i] -= 360;
      outputs[i] = TO_INT(SIN((int)sinAngle[i]) * HALF_MAX_OUTPUT) + HALF_MAX_OUTPUT;
    }
  }

  for (int i = 0; i < 3; i++) {
    pcaselect(i);
    for (int j = 0; j < 12; j++)
      if (dacMapping[j] == i)
         mcp[i].setChannelValue(channelMapping[j], outputs[j]);
  }
}
