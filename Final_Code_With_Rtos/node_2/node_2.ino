#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

// FreeRTOS includes
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h> // For inter-task communication

// OLED Display defines
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset)

// CAN defines
#define CAN_NODE_2_ID 0x040
#define CAN_CS_PIN 5 // Chip Select pin for MCP2515 (adjust if different on your board)

// Sensor and LED pins
#define DOOR_SENSOR_PIN 26
#define RED_LED_PIN 25
#define GREEN_LED_PIN 33
#define BLUE_LED_PIN 32

// HX711 pins
#define DOUT 4
#define CLK 15

// Global objects (accessible by tasks)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MCP2515 mcp2515(CAN_CS_PIN);
HX711 scale;

// Queue for sending sensor data from sensor task to CAN task
QueueHandle_t sensorDataQueue;

// Structure to hold sensor data for inter-task communication
typedef struct {
  bool doorOpen;
  float weight;
} SensorData_t;

// --- Task: Sensor Reading and OLED Display Update ---
// This task reads the door sensor and HX711, updates the OLED,
// and sends the data to the CAN task via a FreeRTOS queue.
void sensor_display_task(void *pvParameters) {
  (void)pvParameters; 

  // Set up HX711 scale calibration and tare
  scale.set_scale(370); // Adjust calibration factor as needed for your specific load cell
  scale.tare();         

  // Display initial message on OLED
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("Node 2 Starting"));
  display.display();
  vTaskDelay(pdMS_TO_TICKS(1000)); 
  // Delay for 1 second using FreeRTOS non-blocking delay
  display.clearDisplay();

  for (;;) {
    // Read door sensor state (LOW means contact, assuming pull-up and switch closes to GND)
    bool doorOpen = (digitalRead(DOOR_SENSOR_PIN) == LOW);
    // Read weight from HX711
    float weight = scale.get_units();

    // Update OLED display with current sensor readings
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 20);
    display.print(F("D:"));
    // Display "CLOSED" if doorOpen is true (sensor LOW), "OPEN" otherwise
    if (doorOpen) {
      display.println(F("CLOSED"));
    } else {
      display.println(F("OPEN"));
    }

    display.setCursor(0, 45);
    display.print(F("W:"));
    display.print(weight, 1); // Display weight with 1 decimal place
    display.display();

    // Prepare sensor data to be sent to the CAN task
    SensorData_t dataToSend;
    dataToSend.doorOpen = doorOpen;
    dataToSend.weight = weight;

    // Send data to the CAN task via the queue.
    // If the queue is full, it will block for a short period (10 ticks)
    // or return pdFALSE if it cannot send, preventing indefinite blocking.
    if (xQueueSend(sensorDataQueue, &dataToSend, pdMS_TO_TICKS(10)) != pdPASS) {
      Serial.println(F("Failed to send sensor data to CAN queue"));
    }

    // Print sensor data to Serial for debugging
    Serial.print(F("Sensor Task - Door: "));
    Serial.print(doorOpen ? F("CLOSED") : F("OPEN"));
    Serial.print(F(", Weight: "));
    Serial.println(weight, 1);

    // Task delay to control update frequency (every 500ms)
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// --- Task: CAN Message Sending ---
// This task waits for sensor data from the sensor_display_task via a queue,
// then constructs and sends a CAN message.
void can_send_task(void *pvParameters) {
  (void)pvParameters; // Cast to void to suppress unused parameter warning

  for (;;) {
    SensorData_t receivedData;

    // Wait indefinitely for data from the sensor task via the queue.
    // This task will block until data is available, consuming no CPU cycles while waiting.
    if (xQueueReceive(sensorDataQueue, &receivedData, portMAX_DELAY) == pdPASS) {
      struct can_frame canMsg;
      canMsg.can_id = CAN_NODE_2_ID;
      canMsg.can_dlc = 5; // Data Length Code: 1 byte for door state + 4 bytes for float weight

      canMsg.data[0] = receivedData.doorOpen ? 1 : 0; // 1 if door is "CLOSED" (sensor LOW), 0 if "OPEN"

      // Copy the float weight bytes into the CAN data buffer
      memcpy(&canMsg.data[1], &receivedData.weight, sizeof(float));

      // Send the CAN message
      if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
        Serial.print(F("CAN Task - Sent Door state: "));
        Serial.print(receivedData.doorOpen ? F("CLOSED") : F("OPEN"));
        Serial.print(F(", Weight: "));
        Serial.println(receivedData.weight, 1);
      } else {
        Serial.println(F("CAN Task - Failed to send CAN message"));
      }
    }
  }
}

// --- Arduino Setup Function ---
// This function initializes hardware and creates the FreeRTOS tasks.
void setup() {
  Serial.begin(115200); // Initialize Serial communication for debugging
  SPI.begin();          // Initialize SPI bus for MCP2515

  // Initialize OLED display (SSD1306)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is the common I2C address for SSD1306
    Serial.println(F("SSD1306 allocation failed"));
    while (true); // Halt program if display initialization fails
  }

  // Initialize LED pins as outputs and set them OFF
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);

  // Initialize MCP2515 CAN controller
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Set CAN bitrate to 500kbps with 8MHz crystal
  mcp2515.setNormalMode();                   // Set to normal operation mode to send/receive messages

  // Initialize Door Sensor pin with internal pull-up resistor
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

  // Initialize HX711 scale (HX711.begin is called here, but calibration/tare in task)
  scale.begin(DOUT, CLK);

  // Test CAN connection by sending a dummy message and update LED status
  bool canConnected = false;
  struct can_frame testMsg;
  testMsg.can_id = CAN_NODE_2_ID;
  testMsg.can_dlc = 1;
  testMsg.data[0] = 0;

  if (mcp2515.sendMessage(&testMsg) == MCP2515::ERROR_OK) {
    canConnected = true;
  }

  if (canConnected) {
    digitalWrite(RED_LED_PIN, HIGH); // Red LED ON if CAN connection is successful
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println(F("CAN Connected - RED LED ON"));
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH); // Green LED ON if CAN connection fails
    Serial.println(F("CAN NOT Connected - GREEN LED ON"));
  }

  // Create the FreeRTOS queue for sensor data.
  // It can hold up to 5 SensorData_t items.
  sensorDataQueue = xQueueCreate(5, sizeof(SensorData_t));
  if (sensorDataQueue == NULL) {
    Serial.println(F("Failed to create sensorDataQueue"));
    while (true); // Halt if queue creation fails (critical error)
  }

  // Create FreeRTOS tasks and pin them to specific CPU cores
  xTaskCreatePinnedToCore(
    sensor_display_task,    // Task function to run
    "SensorDisplayTask",    // Name of the task (for debugging)
    4096,                   // Stack size in bytes (adjust as needed, 4KB is a good start)
    NULL,                   // Parameter to pass to the task function (none in this case)
    1,                      // Task priority (0 is the lowest, configMAX_PRIORITIES-1 is the highest)
    NULL,                   // Task handle (not needed for this example)
    0                       // Core to run on (Core 0)
  );

  xTaskCreatePinnedToCore(
    can_send_task,          // Task function to run
    "CanSendTask",          // Name of the task
    4096,                   // Stack size in bytes
    NULL,                   // Parameter to pass
    2,                      // Task priority (higher than sensor task to ensure CAN messages are sent promptly)
    NULL,                   // Task handle
    1                       // Core to run on (Core 1)
  );

}

void loop() {
 
}
