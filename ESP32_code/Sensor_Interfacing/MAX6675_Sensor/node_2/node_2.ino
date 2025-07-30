#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MCP2515 mcp2515(5);
#define CAN_NODE_2_ID 0x038

unsigned long lastSendTime = 0;
#define SEND_INTERVAL 500
#define DOOR_SENSOR_PIN 26
#define RED_LED_PIN 25
#define GREEN_LED_PIN 33
#define BLUE_LED_PIN 32

void setup() {
  Serial.begin(115200);
  SPI.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("Node 2 Starting"));
  display.display();
  delay(1000);
  display.clearDisplay();

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

  randomSeed(analogRead(0));

  bool canConnected = false;
  struct can_frame testMsg;
  testMsg.can_id = CAN_NODE_2_ID;
  testMsg.can_dlc = 1;
  testMsg.data[0] = 0;

  if (mcp2515.sendMessage(&testMsg) == MCP2515::ERROR_OK) {
    canConnected = true;
  }
  if (canConnected) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println(F("CAN Connected - RED LED ON"));
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println(F("CAN NOT Connected - GREEN LED ON"));
  }
}

void loop() {

  bool doorOpen = (digitalRead(DOOR_SENSOR_PIN) == LOW);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print(F("D:"));
  if (doorOpen) {
    display.println(F("CLOSED"));
  } else {
    display.println(F("OPEN"));
  }
  display.display();

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();

    struct can_frame canMsg;
    canMsg.can_id = CAN_NODE_2_ID;
    canMsg.can_dlc = 1;

    canMsg.data[0] = doorOpen ? 1 : 0;

    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.print(F("Sent Door state: "));
      Serial.println(doorOpen ? F("OPEN") : F("CLOSED"));
    } else {
      Serial.println(F("Failed to send CAN message"));
    }
  }
}
