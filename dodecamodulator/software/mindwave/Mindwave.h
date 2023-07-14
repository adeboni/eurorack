#ifndef Mindwave_h
#define Mindwave_h

#include "Arduino.h"
#include "BluetoothSerial.h"

class Mindwave
{
public:
  uint8_t poorQuality = 100;
  uint8_t attention = 0;
  uint8_t meditation = 0;
  float delta = 0;
  float theta = 0;
  float lowAlpha = 0;
  float highAlpha = 0;
  float lowBeta = 0;
  float highBeta = 0;
  float lowGamma = 0;
  float midGamma = 0;
  int16_t eeg = 0;
  bool hasNewData = false;

  void setup();
  bool connect(uint8_t *address);
  void disconnect();
  void update();
    
private:
  uint8_t readOneByte();
  uint8_t readFirstByte();
  uint8_t payloadData[64] = {0};

  void readWaves(int i);
  uint32_t read3byteInt(int i);
  int16_t read2byteInt(int i);
};

#endif
