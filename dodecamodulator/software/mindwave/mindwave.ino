#include "Mindwave.h"
#include <EEPROM.h>
#include <Adafruit_MCP4728.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Encoder.h>
#include <Bounce2.h>

#define SELECT_BTN      33
#define DOWN_BTN        34
#define UP_BTN          35

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

#define MAX_OUTPUT      2048
#define UPDATE_RATE_US  10000

Adafruit_MCP4728 mcp[3];
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Encoder encoder(UP_BTN, DOWN_BTN);
Bounce button = Bounce();
int selectionIndex = -1;

float dataBuffer1[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float dataBuffer2[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int gateThreshold = 50;
int gateTarget = 0;
char* outputNames[] = {"ATT", "MED", "DLT", "THT", "L-A", "H-A", "L-B", "H-B", "L-G", "M-G", "EEG"};
int outputs[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int dacMapping[] = { 2, 1, 0, 0, 2, 2, 1, 0, 2, 1, 1, 0 };
MCP4728_channel_t channelMapping[] = { 
  MCP4728_CHANNEL_D, MCP4728_CHANNEL_D, MCP4728_CHANNEL_D, MCP4728_CHANNEL_A, 
  MCP4728_CHANNEL_C, MCP4728_CHANNEL_A, MCP4728_CHANNEL_A, MCP4728_CHANNEL_B,
  MCP4728_CHANNEL_B, MCP4728_CHANNEL_C, MCP4728_CHANNEL_B, MCP4728_CHANNEL_C 
};

char addressChars[] = "C464E3EB8431";
bool btConnected = false;
Mindwave mindwave;

void pcaselect(const byte channel) {
  Wire.beginTransmission(0x70);
  Wire.write(0x04 | channel);
  Wire.endTransmission();
}

void saveValues() {
  EEPROM.begin(64);
  int address = 0;
  int dataSaved = 1;
  EEPROM.write(address, dataSaved);
  address += sizeof(dataSaved);
  EEPROM.writeString(address, addressChars);
  address += sizeof(byte) * 13;
  EEPROM.write(address, gateThreshold);
  address += sizeof(gateThreshold);
  EEPROM.write(address, gateTarget);
  address += sizeof(gateTarget);
  EEPROM.commit();
  EEPROM.end();
}

void loadValues() {  
  EEPROM.begin(64);
  int address = 0;
  int dataSaved = EEPROM.read(address);
  address += sizeof(dataSaved);
  if (dataSaved == 1) {
    EEPROM.readString(address).toCharArray(addressChars, 13);
    address += sizeof(byte) * 13;
    gateThreshold = EEPROM.read(address);
    address += sizeof(gateThreshold);
    gateTarget = EEPROM.read(address);
    address += sizeof(gateTarget);
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

void updateUI() {
  pcaselect(3);
  display.clearDisplay();

  if (btConnected) {
    display.drawRect(1, 1, SCREEN_WIDTH - 1, 9, SH110X_WHITE);
    display.fillRect(1, 1, (SCREEN_WIDTH * (100 - mindwave.poorQuality)) / 100 - 1, 9, SH110X_WHITE);
  } else {
    display.setCursor(27, 4);
    display.print("NOT CONNECTED");
  }

  display.setCursor(14, 14);
  for (int i = 0; i < 12; i++) {
   if (selectionIndex == i)
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print(addressChars[i]);
    display.setTextColor(SH110X_WHITE); 
    
    if (i % 2 == 1 && i != 11)
      display.print(':');
  }
  display.setCursor(45, 23);

  if (selectionIndex == 12)
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.print("Connect");
  display.setTextColor(SH110X_WHITE); 

  display.setCursor(8, 35);
  display.print("Gate Target:    ");
  if (selectionIndex == 13)
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.print(outputNames[gateTarget]);
  display.setTextColor(SH110X_WHITE); 

  display.setCursor(8, 44);
  display.print("Gate Threshold: ");
  if (selectionIndex == 14)
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.print(gateThreshold);
  display.setTextColor(SH110X_WHITE); 

  display.drawRect(1, 54, SCREEN_WIDTH - 1, 9, SH110X_WHITE);
  display.fillRect(1, 54, (SCREEN_WIDTH * dataBuffer1[gateTarget]) / 100 - 1, 9, SH110X_WHITE);
  display.drawLine((SCREEN_WIDTH * gateThreshold) / 100 - 1, 55, (SCREEN_WIDTH * gateThreshold) / 100 - 1, 61, 
    dataBuffer1[gateTarget] > gateThreshold ? SH110X_BLACK : SH110X_WHITE);

  display.display();
}

void updateVals() {
  static unsigned long lastTime = 0;
  unsigned long newTime = micros();
  if (newTime - lastTime < UPDATE_RATE_US)
    return;
  lastTime = newTime;

  for (int i = 0; i < 10; i++) {
    dataBuffer1[i] += (dataBuffer2[i] - dataBuffer1[i]) / 30;
    outputs[i] = map(constrain((int)dataBuffer1[i], 0, 100), 0, 100, 0, MAX_OUTPUT);
  }

  outputs[10] = map(constrain((int)dataBuffer1[10], 0, 100), 0, 100, 0, MAX_OUTPUT);
  outputs[11] = dataBuffer1[gateTarget] > gateThreshold ? MAX_OUTPUT : 0;
  
  for (int i = 0; i < 3; i++) {
    pcaselect(i);
    for (int j = 0; j < 12; j++)
      if (dacMapping[j] == i)
         mcp[i].setChannelValue(channelMapping[j], outputs[j]);
  }
}

void connectToBT() {
  displayMessage("Connecting...");

  uint8_t address[6];
  for (int i = 0; i < 6; i++)
    sscanf(addressChars + 2*i, "%02x", &address[i]);

  if (btConnected) {
    mindwave.disconnect();
    btConnected = false;
  }
  
  btConnected = mindwave.connect(address);
  updateUI();
}

void setup() {
  loadValues();
  saveValues();
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
  
  for (int i = 0; i < 3; i++) {
    pcaselect(i);
    if (!mcp[i].begin()) {
      displayMessage("DAC Failure");
      while (1);
    }
  }

  mindwave.setup();
  connectToBT();
}

void updateAddressChar(int index, int dir) {
  if (addressChars[index] == '9' && dir == 1)
    addressChars[index] = 'A';
  else if (addressChars[index] == 'A' && dir == -1)
    addressChars[index] = '9';
  else if (addressChars[index] >= '0' && addressChars[index] <= '9')
    addressChars[index] = constrain(addressChars[index] + dir, (byte)'0', (byte)'9');
  else
    addressChars[index] = constrain(addressChars[index] + dir, (byte)'A', (byte)'F');
}

void readEncoder() {
  static int pos = 0;
  int dir = 0;
  button.update();
  int newPos = encoder.read() / 4;
  if (pos != newPos) {
    dir = newPos > pos ? 1 : -1;
    pos = newPos;
  }

  if (button.read() == LOW && selectionIndex == 12) {
    connectToBT();
  } else if (dir != 0)  {
    if (button.read() == LOW) {
      if (selectionIndex >= 0 && selectionIndex <= 11) {
        updateAddressChar(selectionIndex, dir);
      } else if (selectionIndex == 13) {
        gateTarget = constrain(gateTarget + dir, 0, 10);
      } else if (selectionIndex == 14) {
        gateThreshold = constrain(gateThreshold + dir, 0, 100);
      }
      saveValues();
    } else {
      selectionIndex = constrain(selectionIndex + dir, -1, 14);
    }
    
    updateUI();
  }
}

void loop() {
  mindwave.update();
  dataBuffer1[10] = dataBuffer2[10] = mindwave.eeg;

  if (mindwave.hasNewData) {
    memcpy(dataBuffer1, dataBuffer2, sizeof(dataBuffer1));
    
    dataBuffer2[0] = mindwave.attention;
    dataBuffer2[1] = mindwave.meditation;
    dataBuffer2[2] = mindwave.delta;
    dataBuffer2[3] = mindwave.theta;
    dataBuffer2[4] = mindwave.lowAlpha;
    dataBuffer2[5] = mindwave.highAlpha;
    dataBuffer2[6] = mindwave.lowBeta;
    dataBuffer2[7] = mindwave.highBeta;
    dataBuffer2[8] = mindwave.lowGamma;
    dataBuffer2[9] = mindwave.midGamma;
    
    updateUI();
  } 

  readEncoder();
    updateVals();
}
