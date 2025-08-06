#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "max6675.h"

// RTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Hardware definitions
MCP2515 mcp2515(5);
LiquidCrystal_I2C lcd(0x27, 16, 2);
MAX6675 thermocouple(14, 15, 12);  // CLK, CS, DO
#define WATER_SENSOR_PIN 4
#define RED_LED_PIN 27
#define GREEN_LED_PIN 26

// CAN bus definitions
#define CAN_NODE_1_ID 0x036
#define CAN_SEND_INTERVAL_MS 500

// Shared variables
float temperature_C = 0.0;
bool waterDetected = false;

// Task handles
TaskHandle_t readSensorTaskHandle = NULL;
TaskHandle_t lcdUpdateTaskHandle = NULL;
TaskHandle_t canSendTaskHandle = NULL;


// Task 1: Read Sensor Data
// Reads the temperature from the thermocouple and the water sensor state.

void readSensorTask(void* pvParameters) {
  for (;;) {
    temperature_C = thermocouple.readCelsius();
    waterDetected = (digitalRead(WATER_SENSOR_PIN) == LOW);
    Serial.printf("Sensor Reading: Temp=%.2f°C, Water=%s\n", temperature_C, waterDetected ? "Empty" : "Full");
    vTaskDelay(pdMS_TO_TICKS(1000));  // Read sensors every 1000 ms
  }
}

// Task 2: Update LCD Display
// Updates the LCD with the latest sensor data.

void lcdUpdateTask(void* pvParameters) {
  lcd.init();
  lcd.backlight();
  for (;;) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Engine T:");
    lcd.print(temperature_C, 2);
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Oil:");
    lcd.print(waterDetected ? "Empty " : "Full ");
    vTaskDelay(pdMS_TO_TICKS(500));  // Update LCD every 500 ms
  }
}


// Task 3: Send CAN Messages
// Prepares and sends CAN messages containing sensor data.
void canSendTask(void* pvParameters) {
  struct can_frame canMsg;
  canMsg.can_id = CAN_NODE_1_ID;
  canMsg.can_dlc = 3;

  for (;;) {
    int16_t tempSend = (int16_t)(temperature_C * 100);
    canMsg.data[0] = (tempSend >> 8) & 0xFF;
    canMsg.data[1] = tempSend & 0xFF;
    canMsg.data[2] = waterDetected ? 1 : 0;

    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.println("CAN message sent successfully.");
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, LOW);
    } else {
      Serial.println("Failed to send CAN message.");
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);
    }
    vTaskDelay(pdMS_TO_TICKS(CAN_SEND_INTERVAL_MS));
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, HIGH);  // Assume error state initially
  digitalWrite(GREEN_LED_PIN, LOW);

  pinMode(WATER_SENSOR_PIN, INPUT_PULLUP);

  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("CAN Node 1 started - RTOS version");
  Serial.println("Creating RTOS tasks...");
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  // Create tasks
  xTaskCreate(
    readSensorTask,        // Task function
    "Read Sensor Task",    // Name of the task
    2048,                  // Stack size in words
    NULL,                  // Parameter to pass to the task
    1,                     // Priority of the task
    &readSensorTaskHandle  // Task handle
  );

  xTaskCreate(
    lcdUpdateTask,        // Task function
    "LCD Update Task",    // Name of the task
    2048,                 // Stack size
    NULL,                 // Parameter
    1,                    // Priority
    &lcdUpdateTaskHandle  // Task handle
  );

  xTaskCreate(
    canSendTask,        // Task function
    "CAN Send Task",    // Name of the task
    2048,               // Stack size
    NULL,               // Parameter
    1,                  // Priority
    &canSendTaskHandle  // Task handle
  );
}


void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));  // The loop() can be an idle task
}
