#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_FreeRTOS.h> // or other FreeRTOS include for your platform

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MCP2515 mcp2515(5);
#define CAN_NODE_2_ID 0x038

#define DOOR_SENSOR_PIN 26
#define RED_LED_PIN 25
#define GREEN_LED_PIN 33
#define BLUE_LED_PIN 32

#define SEND_INTERVAL pdMS_TO_TICKS(500)  // Convert ms to RTOS ticks

// Shared variable for door state
volatile bool doorOpen = false;

void canTask(void *pvParameters) {
  struct can_frame canMsg;
  canMsg.can_id = CAN_NODE_2_ID;
  canMsg.can_dlc = 1;

  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    canMsg.data[0] = doorOpen ? 1 : 0;

    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.print(F("Sent Door state: "));
      Serial.println(doorOpen ? F("OPEN") : F("CLOSED"));
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);
    } else {
      Serial.println(F("Failed to send CAN message"));
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);
    }

    vTaskDelayUntil(&lastWakeTime, SEND_INTERVAL);
  }
}

void displayTask(void *pvParameters) {
  for (;;) {
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

    vTaskDelay(pdMS_TO_TICKS(200)); // Update display every 200 ms
  }
}

void sensorTask(void *pvParameters) {
  for (;;) {
    bool currentDoorState = (digitalRead(DOOR_SENSOR_PIN) == LOW);
    // Update shared state atomically if needed, but bool is atomic on ESP32
    doorOpen = currentDoorState;

    vTaskDelay(pdMS_TO_TICKS(100)); // Check door sensor every 100ms
  }
}

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

  // Create RTOS tasks
  xTaskCreate(sensorTask, "Sensor Task", 128, NULL, 2, NULL);
  xTaskCreate(displayTask, "Display Task", 256, NULL, 1, NULL);
  xTaskCreate(canTask, "CAN Task", 256, NULL, 3, NULL);

  // Start RTOS scheduler
  vTaskStartScheduler();

  // Will not reach here unless scheduler fails
  Serial.println(F("RTOS scheduler failed!"));
}

void loop() {
  // Empty: control taken by tasks
}
