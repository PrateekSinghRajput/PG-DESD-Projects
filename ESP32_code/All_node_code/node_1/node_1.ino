#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "max6675.h"

MCP2515 mcp2515(5);
#define CAN_NODE_1_ID 0x036
unsigned long lastSendTime = 0;
#define SEND_INTERVAL 500  


int thermoDO = 12;
int thermoCS = 15;
int thermoCLK = 14;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define WATER_SENSOR_PIN 4
bool waterDetected = false;

#define RED_LED_PIN 27
#define GREEN_LED_PIN 26

bool canConnected = false;

void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);


  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("........");
  lcd.setCursor(0, 1);
  lcd.print("........");
  delay(2000);

  pinMode(WATER_SENSOR_PIN, INPUT_PULLUP);

  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  canConnected = true;

  if (canConnected) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    Serial.println("CAN module assumed connected - GREEN LED ON");
  } else {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println("CAN module NOT connected - RED LED ON");
  }

  Serial.println("Node 1 started");
}

void loop() {
  float temp_C = thermocouple.readCelsius();
  float temp_F = thermocouple.readFahrenheit();

  waterDetected = (digitalRead(WATER_SENSOR_PIN) == LOW);

  Serial.print("°C = ");
  Serial.println(temp_C);
  Serial.print("°F = ");
  Serial.println(temp_F);
  Serial.print("Water Detected: ");
  Serial.println(waterDetected ? "Empty" : "Full");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Engine T:");
  lcd.print(temp_C, 2);
  lcd.print((char)223);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Oil:");
  lcd.print(waterDetected ? "Empty " : "Full ");

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();

    struct can_frame canMsg;
    canMsg.can_id = CAN_NODE_1_ID;
    canMsg.can_dlc = 3;

    int16_t tempSend = (int16_t)(temp_C * 100);
    canMsg.data[0] = (tempSend >> 8) & 0xFF;
    canMsg.data[1] = tempSend & 0xFF;

    canMsg.data[2] = waterDetected ? 1 : 0;

    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.print("Sent CAN: Temp=");
      Serial.print(temp_C, 2);
      Serial.print(" C, Water=");
      Serial.println(waterDetected ? "YES" : "NO");
    } else {
      Serial.println("Failed to send CAN message");

      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);
    }
  }

  delay(1000);
}
