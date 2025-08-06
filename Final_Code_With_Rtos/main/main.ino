#include <SPI.h>
#include <mcp2515.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <WebServer.h> 
#include <ThingSpeak.h> 

#include "display.h"
#include "webserver.h"
#include "thingspeak.h"
#include "globals.h"
#include "can_ids.h"

// Pins for TFT display
#define TFT_CS 2
#define TFT_DC 4
#define TFT_RST 15

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
MCP2515 mcp2515(5);

// LEDs and buzzer
#define BUZZER_PIN 25
#define YELLOW_LED_PIN 27
#define RED_LED_PIN 26
#define GREEN_LED_PIN 14

// WiFi credentials
const char *ssid = "PrateekSin";
const char *password = "1234512345";

// Web server port
WebServer server(80);

// ThingSpeak client
WiFiClient client;

// Timing for ThingSpeak update
unsigned long lastThingSpeakUpdate = 0;
const unsigned long THINGSPEAK_UPDATE_INTERVAL = 15000;  // 15 seconds

void setup() {
  Serial.begin(115200);
  SPI.begin();

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  drawHeader();
  drawCANStatus(false);
  drawNode1(0, false);
  drawNode2(false, 0.0, 12.971598, 77.594566);

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

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  delay(3000);  // Stabilize before ThingSpeak

  ThingSpeak.begin(client);

  setupWebServer(server);

  server.begin();
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
      digitalWrite(YELLOW_LED_PIN, waterEmpty ? HIGH : LOW);

      webTemp = temperature;
      webWaterEmpty = waterEmpty;

    } else if (canMsg.can_id == CAN_NODE_2_ID && canMsg.can_dlc >= 5) {
      bool doorOpen = (canMsg.data[0] == 1);
      float loadWeight;
      memcpy(&loadWeight, &canMsg.data[1], sizeof(float));

      if (doorOpen != lastDoor) {
        drawNode2(doorOpen, loadWeight, node3Latitude, node3Longitude);
        lastDoor = doorOpen;
      }

      node2Load = loadWeight;
      webDoorOpen = doorOpen;
      webLoad = loadWeight;

      digitalWrite(GREEN_LED_PIN, doorOpen ? LOW : HIGH);

    } else if (canMsg.can_id == CAN_NODE_3_ID && canMsg.can_dlc >= 8) {
      float lat_val, lon_val;
      memcpy(&lat_val, canMsg.data, sizeof(float));
      memcpy(&lon_val, &canMsg.data[4], sizeof(float));

      node3Latitude = lat_val;
      node3Longitude = lon_val;

      webLatitude = lat_val;
      webLongitude = lon_val;

      drawNode2(lastDoor, node2Load, node3Latitude, node3Longitude);
    }
  }

  drawCANStatus((millis() - lastCANMsgTime) < CAN_TIMEOUT);
  drawNode2(lastDoor, node2Load, node3Latitude, node3Longitude);

  server.handleClient();

  unsigned long now = millis();
  if (now - lastThingSpeakUpdate >= THINGSPEAK_UPDATE_INTERVAL) {
    updateThingSpeak(webTemp, webWaterEmpty, webDoorOpen, webLatitude, webLongitude, webLoad, ssid, password);
    lastThingSpeakUpdate = now;
  }

  delay(200);
}
