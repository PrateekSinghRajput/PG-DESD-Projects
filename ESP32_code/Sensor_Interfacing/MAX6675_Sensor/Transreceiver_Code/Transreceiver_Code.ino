#include <SPI.h>
#include <mcp2515.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS 2
#define TFT_DC 4
#define TFT_RST 15

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

MCP2515 mcp2515(5);

#define CAN_NODE_1_ID 0x036
#define CAN_NODE_2_ID 0x038
#define CAN_NODE_3_ID 0x040

unsigned long lastCANMsgTime = 0;
const unsigned long CAN_TIMEOUT = 2000;

unsigned long lastNode1MsgTime = 0;
unsigned long lastNode2MsgTime = 0;

float lastTemp = -999;
bool lastWater = false;
bool lastDoor = false;

float node3Latitude = 12.971598;
float node3Longitude = 77.594566;
float node3Load = 0.0;

#define BUZZER_PIN 25
#define YELLOW_LED_PIN 27
#define RED_LED_PIN 26
#define GREEN_LED_PIN 14

void drawHeader() {
  tft.fillRect(0, 0, 160, 20, ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setCursor(10, 2);
  tft.print("CAN Rx");
}

void drawNode1(float temp, bool waterEmpty) {
  tft.fillRect(0, 22, 160, 38, ST77XX_BLACK);
  tft.drawRect(0, 22, 160, 38, ST77XX_WHITE);

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 28);
  tft.print("Node1");

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(70, 28);
  tft.print("Temp: ");
  tft.print(temp, 1);
  tft.print("C");

  tft.setCursor(70, 40);
  tft.print("Oil: ");
  tft.setTextColor(waterEmpty ? ST77XX_RED : ST77XX_GREEN, ST77XX_BLACK);
  tft.print(waterEmpty ? "Empty" : "Full ");
}

void drawNode2(bool doorOpen) {
  tft.fillRect(0, 62, 160, 38, ST77XX_BLACK);
  tft.drawRect(0, 62, 160, 38, ST7735_WHITE);

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 68);
  tft.print("Node2");

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(70, 68);
  tft.print("Door: ");
  tft.setTextColor(doorOpen ? ST77XX_RED : ST77XX_GREEN, ST77XX_BLACK);
  tft.print(doorOpen ? "CLOSED" : "OPEN");
}

void drawCANStatus(bool canOK) {
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(115, 6);
  tft.print(canOK ? "CAN OK" : "NO CAN");
}

// Draw Node 3 GPS + Load sensor data in lower area
void drawNode3(float latitude, float longitude, float load) {
  tft.fillRect(0, 102, 110, 58, ST77XX_BLACK);
  tft.drawRect(0, 102, 160, 58, ST77XX_WHITE);

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 108);
  tft.print("Node3");

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(70, 108);
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  tft.print("Load: ");
  tft.print(load, 2);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(6, 118);
  //tft.print("Lat: ");
  tft.print(latitude, 6);

  tft.setCursor(70, 118);
  //tft.print("Lon: ");
  tft.print(longitude, 6);
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  drawHeader();
  drawCANStatus(false);
  drawNode1(0, false);
  drawNode2(false);
  drawNode3(node3Latitude, node3Longitude, node3Load);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("MCP2515 Initialized and Normal Mode Set");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void loop() {
  struct can_frame canMsg;

  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    lastCANMsgTime = millis();

    if (canMsg.can_id == CAN_NODE_1_ID) lastNode1MsgTime = millis();
    if (canMsg.can_id == CAN_NODE_2_ID) lastNode2MsgTime = millis();

    if (canMsg.can_id == CAN_NODE_1_ID && canMsg.can_dlc == 3) {
      int16_t tempRaw = ((int16_t)canMsg.data[0] << 8) | canMsg.data[1];
      float temperature = tempRaw / 100.0;
      bool waterEmpty = (canMsg.data[2] == 1);

      if (temperature != lastTemp || waterEmpty != lastWater) {
        drawNode1(temperature, waterEmpty);
        lastTemp = temperature;
        lastWater = waterEmpty;
      }

      if (temperature > 35.0) {
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(RED_LED_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(RED_LED_PIN, LOW);
      }

      if (waterEmpty) {
        digitalWrite(YELLOW_LED_PIN, HIGH);
      } else {
        digitalWrite(YELLOW_LED_PIN, LOW);
      }
    } else if (canMsg.can_id == CAN_NODE_2_ID && canMsg.can_dlc == 1) {
      bool doorOpen = (canMsg.data[0] == 1);

      if (doorOpen != lastDoor) {
        drawNode2(doorOpen);
        lastDoor = doorOpen;
      }

      if (doorOpen) {
        digitalWrite(GREEN_LED_PIN, LOW);
      } else {
        digitalWrite(GREEN_LED_PIN, HIGH);
      }
    } else if (canMsg.can_id == CAN_NODE_3_ID) {
      if (canMsg.can_dlc >= 8) {
        float lat_val, lon_val;
        uint8_t latBytes[4], lonBytes[4];
        for (int i = 0; i < 4; i++) {
          latBytes[i] = canMsg.data[i];
          lonBytes[i] = canMsg.data[i + 4];
        }
        memcpy(&lat_val, latBytes, sizeof(float));
        memcpy(&lon_val, lonBytes, sizeof(float));

        node3Latitude = lat_val;
        node3Longitude = lon_val;
      }
    }
  }

  drawCANStatus((millis() - lastCANMsgTime) < CAN_TIMEOUT);
  drawNode3(node3Latitude, node3Longitude, node3Load);

  delay(200);
}
