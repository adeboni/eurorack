#include "Mindwave.h"

BluetoothSerial SerialBT;

void Mindwave::setup(){
  SerialBT.begin("EEG", true); 
  SerialBT.setPin("0000");
}

void Mindwave::disconnect() {
  SerialBT.disconnect();
}

bool Mindwave::connect(uint8_t *address) {
   if (SerialBT.connect(address)) {
    SerialBT.write(0x02);
    return true;
  } 

  return false;
}

uint8_t Mindwave::readFirstByte() {
  if (SerialBT.available())
    return SerialBT.read();
  return 0x00;
}

uint8_t Mindwave::readOneByte() {
  while (!SerialBT.available()) ;
  return SerialBT.read();
}

void Mindwave::readWaves(int i) {
  delta = read3byteInt(i);
  i += 3;
  theta = read3byteInt(i);
  i += 3;
  lowAlpha = read3byteInt(i);
  i += 3;
  highAlpha = read3byteInt(i);
  i += 3;
  lowBeta = read3byteInt(i);
  i += 3;
  highBeta = read3byteInt(i);
  i += 3;
  lowGamma = read3byteInt(i);
  i += 3;
  midGamma = read3byteInt(i);

  float sum = delta + theta + lowAlpha + highAlpha
      + lowBeta + highBeta + lowGamma + midGamma;

  delta = 100 * delta / sum;
  theta = 100 * theta / sum;
  lowAlpha = 100 * lowAlpha / sum;
  highAlpha = 100 * highAlpha / sum;
  lowBeta = 100 * lowBeta / sum;
  highBeta = 100 * highBeta / sum;
  lowGamma = 100 * lowGamma / sum;
  midGamma = 100 * midGamma / sum;
}

uint32_t Mindwave::read3byteInt(int i) {
  return ((payloadData[i] << 16) + (payloadData[i+1] << 8) + payloadData[i+2]);
}

int16_t Mindwave::read2byteInt(int i) {
  return (payloadData[i] << 8) + payloadData[i+1];
}

void Mindwave::update(){
  hasNewData = false;

  if (readFirstByte() == 170) {
    if (readOneByte() == 170) {

      uint8_t payloadLength = readOneByte();
      if (payloadLength > 169) return;
      
      uint8_t generatedChecksum = 0;        
      for (int i = 0; i < payloadLength; i++) {  
        payloadData[i] = readOneByte();
        generatedChecksum += payloadData[i];
      }   

      uint8_t checksum = readOneByte();  
      generatedChecksum = 255 - generatedChecksum;
      
      if (checksum == generatedChecksum) {
        poorQuality = 100;
        uint8_t vlen = 0;

        for(int i = 0; i < payloadLength; i++) {
          switch (payloadData[i]) {
          case 0x02:
            i++;
            poorQuality = min((int)payloadData[i], 200) / 2;
            break;
          case 0x04:
            i++;
            attention = payloadData[i];            
            break;
          case 0x05:
            i++;
            meditation = payloadData[i];
            break;
          case 0x80:
            i++;
            vlen = payloadData[i];
            eeg = map(constrain((int)read2byteInt(i + 1), -500, 500), -500, 500, 0, 100);
            i += vlen;
            break;
          case 0x83:
            i++;
            hasNewData = true;
            vlen = payloadData[i];
            readWaves(i + 1);
            i += vlen;
            break;
          default:
            break;
          } 
        }
      }
    }
  }
}
