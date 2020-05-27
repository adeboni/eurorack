#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Bounce2.h>
#include <RotaryEncoder.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <elapsedMillis.h>

#define NUM_MENU_ITEMS   6
#define NUM_MENU_OPTIONS 6
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define ENCODER_A  0
#define ENCODER_B  1
#define ENCODER_SW 2
#define OLED_D0    3
#define OLED_D1    4
#define OLED_RES   5
#define OLED_DC    6
#define OLED_CS    7
#define LED_DATA   23
#define LED_CLOCK  22
#define LED_LATCH  21

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_D1, OLED_D0, OLED_DC, OLED_RES, OLED_CS);
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

Bounce encoderSwitch = Bounce();
RotaryEncoder encoder(ENCODER_A, ENCODER_B);
int encoderPos = 0;

String localIP = "";
int menuLine = 0;
bool lineSelected = false;
String menuOptions[] = {"Bomb Planted", "Bomb Exploded", "Bomb Defused", "Round Live", "Round Freeze Time", "Round Over"};
bool gameStates[] = {false, false, false, false, false, false};
int menuValues[] = {0, 1, 2, 3, 4, 5};
int outputPins[] = {20, 19, 18, 17, 16, 15};

EthernetServer server(8080);
DynamicJsonDocument jsonData(2048);
elapsedMillis lastUpdate = 0;
int lastScreenRefresh = 0;
bool anyUpdate = false;

void setup() {
  encoderSwitch.attach(ENCODER_SW, INPUT_PULLUP);
  encoderSwitch.interval(5);

  pinMode(LED_DATA, OUTPUT);
  pinMode(LED_CLOCK, OUTPUT);
  pinMode(LED_LATCH, OUTPUT);

  for (int i = 0; i < NUM_MENU_ITEMS; i++)
    pinMode(outputPins[i], OUTPUT);

  for (int i = 0; i < NUM_MENU_ITEMS; i++)
    digitalWrite(outputPins[i], HIGH);

  delay(200);

  for (int i = 0; i < NUM_MENU_ITEMS; i++)
    digitalWrite(outputPins[i], LOW);

  loadMemory();
  updateOutputs();
 
  if (!display.begin(SSD1306_SWITCHCAPVCC))
    delayForever();

  initMenu("    Connecting...");
  if (Ethernet.begin(mac) == 0) {
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      initMenu("  Hardware error");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      initMenu(" No cable connected");
    }
    delayForever();
  }

  localIP = "IP: " + toString(Ethernet.localIP());
  updateMenu();
}

void loop() {
  checkButtons();
  if (checkNetwork())
    updateMenu();
  else if (anyUpdate && lastUpdate > lastScreenRefresh + 1000)
    updateMenu();
}

bool checkNetwork() {
  bool newData = false;
  EthernetClient client = server.available();
  if (client) {
    bool currentLineIsBlank = true;
    
    while (client.connected()) {
      deserializeJson(jsonData, readRequestBody(client));

      gameStates[0] = jsonData["round"]["bomb"].as<String>() == "planted";
      gameStates[1] = jsonData["round"]["bomb"].as<String>() == "exploded";
      gameStates[2] = jsonData["round"]["bomb"].as<String>() == "defused";
      gameStates[3] = jsonData["round"]["phase"].as<String>() == "live";
      gameStates[4] = jsonData["round"]["phase"].as<String>() == "freezetime";
      gameStates[5] = jsonData["round"]["phase"].as<String>() == "over";

      anyUpdate = true;
      lastUpdate = 0;
      newData = true;
      updateOutputs();
      
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      break;
    }

    delay(1);
    client.stop();
  }

  return newData;
}

void checkButtons() {
  encoderSwitch.update();
  encoder.tick();

  int newPos = encoder.getPosition();
  if (encoderPos != newPos) {
    if (encoderPos < newPos) {
      if (!lineSelected) {
        menuLine = max(0, menuLine - 1);
      } else {
        menuValues[menuLine] = max(0, menuValues[menuLine] - 1);
        updateMemory();
      }
    } else {
      if (!lineSelected) {
        menuLine = min(NUM_MENU_ITEMS - 1, menuLine + 1);
      } else {
        menuValues[menuLine] = min(NUM_MENU_OPTIONS - 1, menuValues[menuLine] + 1);
        updateMemory();
      }
    }

    updateMenu();
    encoderPos = newPos;
  }

  if (encoderSwitch.fell()) {
    lineSelected = !lineSelected;
    updateMenu();
  }
}

void updateMemory() {
  for (int i = 0; i < NUM_MENU_ITEMS; i++)
    EEPROM.write(i, menuValues[i]);
  EEPROM.write(NUM_MENU_ITEMS, 1);
}

void loadMemory() {
  for (int i = 0; i < NUM_MENU_ITEMS; i++)
    menuValues[i] = EEPROM.read(i);
  int valid = EEPROM.read(NUM_MENU_ITEMS);
  if (valid != 1) {
    for (int i = 0; i < NUM_MENU_ITEMS; i++)
      menuValues[i] = i;
  }
}

void updateOutputs() {
  bool leds[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (int i = 0; i < NUM_MENU_ITEMS; i++) {
    leds[i + 1] = gameStates[menuValues[i]];
    digitalWrite(outputPins[i], gameStates[menuValues[i]]);
  }

  setLED(leds);
}

void initMenu(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(msg);

  display.display();
}

void updateMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(localIP);
  if (anyUpdate) {
    int secs = lastUpdate / 1000;
    if (secs < 60)
      display.println("Updated " + String(secs) + " sec ago");
    else if (secs < 3600)
      display.println("Updated " + String(secs / 60) + " min ago");
    else
      display.println("Updated " + String(secs / 3600) + " hr ago");
  } else {
    display.println();
  }

  for (int i = 0; i < NUM_MENU_ITEMS; i++) {
    if (menuLine == i && !lineSelected) {
       display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
       display.print(String(i + 1) + ": ");
    }
    else if (menuLine == i && lineSelected) {
       display.print(String(i + 1) + ": ");
       display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }
    else {
      display.setTextColor(SSD1306_WHITE);
      display.print(String(i + 1) + ": ");
    }

    display.println(menuOptions[menuValues[i]]);
  }

  display.display();

  lastScreenRefresh = lastUpdate;
}

String readRequestBody(EthernetClient client)
{
  String res = "";
  bool currentLineIsBlank = true;
  bool foundPayload = false;
  while (client.available())
  {
    char c = client.read();
    
    if (c == '\n' && currentLineIsBlank) 
      foundPayload = true;
    if (c == '\n')
      currentLineIsBlank = true;
    else if (c != '\r')
      currentLineIsBlank = false;

    if (foundPayload)
      res += c;
  }
  return res;
}

void delayForever() {
  while (true)
    delay(1); 
}

String toString(const IPAddress& address){
  return String() + address[0] + "." + address[1] + "." + address[2] + "." + address[3];
}

void setLED(bool values[]) {
  int ledData = 0;
  for(int i = 0; i < 8; i++)
     ledData |= values[i] << i;
  digitalWrite(LED_LATCH, LOW);
  shiftOut(LED_DATA, LED_CLOCK, MSBFIRST, ledData);  
  digitalWrite(LED_LATCH, HIGH);
}
