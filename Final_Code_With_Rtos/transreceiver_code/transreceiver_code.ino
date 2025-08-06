
/*
 * This is a complete C++ program for an ESP32 microcontroller.
 * It acts as a central hub (a "receiver node") that processes data from other nodes on a CAN bus,
 * displays this data on a TFT screen, provides a web-based dashboard, and uploads the data to ThingSpeak.

 * - <SPI.h>: Standard library for SPI communication, used to talk to the MCP2515 CAN controller and the ST7735 TFT display.
 * - <mcp2515.h>: Library for interfacing with the MCP2515 stand-alone CAN controller, which handles the CAN bus communication.
 * - <Adafruit_GFX.h>: The Adafruit Graphics library, which provides a common set of graphics primitives for displays.
 * - <Adafruit_ST7735.h>: A specific library for the ST7735 TFT display, built on top of Adafruit_GFX.
 * - <WiFi.h>: ESP32's built-in library for connecting to a WiFi network.
 * - <WebServer.h>: Library for creating a simple web server on the ESP32.
 * - <ThingSpeak.h>: Library for interacting with the ThingSpeak IoT platform.
 */

#include <SPI.h>
#include <mcp2515.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ThingSpeak.h>

/* Define pins for the ST7735 TFT display */
#define TFT_CS 2    /* Chip Select pin for the TFT display */
#define TFT_DC 4    /* Data/Command pin for the TFT display */
#define TFT_RST 15  /* Reset pin for the TFT display */

/* Define pins for local alerting LEDs and Buzzer */
#define BUZZER_PIN 25
#define YELLOW_LED_PIN 27
#define RED_LED_PIN 26
#define GREEN_LED_PIN 14

/* WiFi credentials */
const char* ssid = "PrateekSin";
const char* password = "1234512345";

/* ThingSpeak variables */
WiFiClient client;
unsigned long lastThingSpeakUpdate = 0;
const unsigned long THINGSPEAK_UPDATE_INTERVAL = 15000; /* Update ThingSpeak every 15 seconds */

#define THINGSPEAK_CHANNEL_ID 3026763 /* Your ThingSpeak Channel ID */
const char* thingspeakApiKey = "I3V58JNL7DM6EG53"; /* Your ThingSpeak Write API Key */

/* Initialize the TFT display object */
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/* Initialize the MCP2515 CAN controller object (SPI CS pin is 5) */
MCP2515 mcp2515(5);

/* Web server object on port 80 */
WebServer server(80);

/* Define CAN IDs for the different nodes */
#define CAN_NODE_1_ID 0x036 /* ID for Node 1 (Temperature, Oil Level) */
#define CAN_NODE_2_ID 0x040 /* ID for Node 2 (Door Status, Load) */
#define CAN_NODE_3_ID 0x038 /* ID for Node 3 (GPS) */

/* Variables to track last received CAN message times for connection status */
unsigned long lastCANMsgTime = 0;
const unsigned long CAN_TIMEOUT = 2000; /* Timeout for CAN messages (2 seconds) */

unsigned long lastNode1MsgTime = 0;
unsigned long lastNode2MsgTime = 0;
unsigned long lastNode3MsgTime = 0;

/* Variables to store last received sensor values for TFT display updates */
float lastTemp = -999;
bool lastWater = false;
bool lastDoor = false;

/* Variables to store sensor values for web server and ThingSpeak */
float node3Latitude = 12.971598; /* Default latitude */
float node3Longitude = 77.594566; /* Default longitude */
float node2Load = 0.0;           /* Default load */

/* Variables to hold current sensor data for the web interface */
float webTemp = 0;
bool webWaterEmpty = false;
bool webDoorOpen = false;
float webLatitude = 12.971598;
float webLongitude = 77.594566;
float webLoad = 0.0;

/* Connection status flags for web interface */
bool node1Connected = false;
bool node2Connected = false;
bool node3Connected = false;

/**
 *  Draws the header on the TFT display.
 * This function fills the top section of the screen with blue and displays "CAN Rx".
 */
void drawHeader() {
  tft.fillRect(0, 0, 160, 20, ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setCursor(10, 2);
  tft.print("CAN Rx");
}

/**
 * Draws Node 1 data (Temperature and Oil Level) on the TFT display.
 *  temp The temperature value to display.
 *  waterEmpty True if the oil tank is empty, false otherwise.
 */
void drawNode1(float temp, bool waterEmpty) {
  /* Clear the previous content for Node 1 section */
  tft.fillRect(0, 22, 160, 38, ST77XX_BLACK);
  /* Draw a white border around the Node 1 section */
  tft.drawRect(0, 22, 160, 38, ST77XX_WHITE);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 28);
  tft.print("Node1");
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(70, 28);
  tft.print("Temp: ");
  tft.print(temp, 1); /* Display temperature with 1 decimal place */
  tft.print("C");
  tft.setCursor(70, 40);
  tft.print("Oil: ");
  /* Change color based on oil level status */
  tft.setTextColor(waterEmpty ? ST77XX_RED : ST77XX_GREEN, ST77XX_BLACK);
  tft.print(waterEmpty ? "Empty" : "Full "); /* Add space for consistent width */
}

/**
 *  Draws Node 2 data (Door Status, Load, and GPS) on the TFT display.
 *  doorOpen True if the door is open, false otherwise.
 * Note: GPS data (node3Latitude, node3Longitude) is merged into this display.
 */
void drawNode2(bool doorOpen) {
  /* Clear the previous content for Node 2 section (and merged Node 3 GPS) */
  tft.fillRect(0, 62, 160, 88, ST77XX_BLACK);
  /* Draw a white border around the section */
  tft.drawRect(0, 62, 160, 88, ST7735_WHITE);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 68);
  tft.print("Node2");
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(70, 68);
  tft.print("Door: ");
  /* Change color based on door status */
  tft.setTextColor(doorOpen ? ST77XX_RED : ST77XX_GREEN, ST77XX_BLACK);
  tft.print(doorOpen ? "CLOSED" : "OPEN");

  tft.setCursor(6, 90);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.print("Load: ");
  tft.print(node2Load, 2); /* Display load with 2 decimal places */

  tft.setCursor(6, 110);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print("GPS:");

  tft.setCursor(40, 110);
  tft.print(node3Latitude, 6); /* Display latitude with 6 decimal places */

  tft.setCursor(40, 122);
  tft.print(node3Longitude, 6); /* Display longitude with 6 decimal places */
}

/**
 *  Draws the overall CAN bus status on the TFT display.
 *  canOK True if CAN messages are being received, false otherwise.
 */
void drawCANStatus(bool canOK) {
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(115, 6);
  tft.print(canOK ? "CAN OK" : "NO CAN");
}

/**
 *  Handles requests to the root URL (/) of the web server.
 * Sends the HTML content for the sensor dashboard.
 */
void handleRoot() {
  /* Raw string literal for the HTML content of the web dashboard */
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
<title>CAN Node Sensor Dashboard</title>
<script src="https://cdn.jsdelivr.net/npm/raphael@2.3.0/raphael.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/justgage@1.3.5/justgage.min.js"></script>
<style>
  /* Import Google Font for a modern look */
  @import url('https://fonts.googleapis.com/css2?family=Roboto&display=swap');

  /* Universal box-sizing for consistent layout */
  * {
    box-sizing: border-box;
  }

  /* Body styling for background, font, text alignment */
  body {
    font-family: 'Roboto', Arial, sans-serif;
    background: #1e1e2f; /* Dark background */
    color: #eee; /* Light text color */
    margin: 0;
    padding: 20px;
    text-align: center;
    -webkit-font-smoothing: antialiased; /* Smoother fonts */
  }

  /* Main heading styling */
  h1 {
    color: #4fc3f7; /* Light blue */
    margin-bottom: 30px;
    text-shadow: 0 0 10px #4fc3f7; /* Glow effect */
    font-weight: 700;
    font-size: 2.5rem;
  }

  /* Container for overall layout */
  .container {
    max-width: 960px;
    margin: 0 auto;
    padding: 0 15px;
  }

  /* Grid layout for cards (2 columns on larger screens) */
  .container-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr); /* Two equal columns */
    gap: 30px; /* Space between cards */
  }

  /* Styling for individual sensor data cards */
  .card {
    background: #2c2c3f; /* Slightly lighter dark background */
    border-radius: 15px; /* Rounded corners */
    padding: 25px 20px;
    box-shadow: 0 0 20px rgba(0,0,0,0.9); /* Deep shadow */
    color: #eee;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 380px; /* Minimum height for consistent card size */
    transition: transform 0.3s ease; /* Smooth hover effect */
  }
  .card:hover {
    transform: translateY(-8px); /* Lift effect on hover */
    box-shadow: 0 0 35px #4fc3f7; /* Enhanced glow on hover */
  }

  /* Card heading styling */
  .card h2 {
    margin-top: 0;
    margin-bottom: 20px;
    color: #4fc3f7;
    font-weight: 700;
    font-size: 1.8rem;
  }

  /* Container for JustGage elements */
  .gauge-container {
    width: 200px;
    height: 160px;
    margin: 0 auto 1rem auto;
  }

  /* Styling for numeric sensor values */
  .value-text {
    font-size: 1.8rem;
    font-weight: 600;
    margin: 10px 0 18px 0;
  }

  /* Styling for badges (e.g., Oil Level, Door Status) */
  .badge {
    font-size: 1.2rem;
    font-weight: 700;
    padding: 6px 24px;
    border-radius: 25px;
    color: white;
    user-select: none; /* Prevent text selection */
    box-shadow: 0 0 8px rgba(0,0,0,0.8);
    margin: 10px 0;
  }
  /* Specific badge colors based on status */
  .badge.full {
    background-color: #4caf50; /* Green */
    box-shadow: 0 0 20px #4caf5080; /* Green glow */
  }
  .badge.empty {
    background-color: #f44336; /* Red */
    box-shadow: 0 0 20px #f4433680; /* Red glow */
  }
  .badge.open {
    background-color: #43a047; /* Darker green */
    box-shadow: 0 0 20px #43a047cc; /* Darker green glow */
  }
  .badge.closed {
    background-color: #e53935; /* Darker red */
    box-shadow: 0 0 20px #e5393577; /* Darker red glow */
  }

  /* Styling for LED connection indicators */
  .led-circle {
    width: 22px;
    height: 22px;
    border-radius: 50%; /* Circular shape */
    background: red; /* Default disconnected color */
    box-shadow: 0 0 12px red; /* Red glow */
    margin-right: 12px;
    display: inline-block;
    vertical-align: middle;
    transition: background-color 0.3s ease, box-shadow 0.3s ease; /* Smooth transition for color change */
  }
  .led-green {
    background: #4caf50; /* Connected color */
    box-shadow: 0 0 15px #4caf50; /* Green glow */
  }

  /* Styling for connection status text */
  .status-item {
    font-size: 1.3rem;
    color: #eee;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 12px;
    margin-top: 15px;
  }

  /* Styling for GPS information block */
  .gps-info {
    margin-top: 15px;
    font-size: 1.3rem;
    text-align: left;
    max-width: 260px;
  }

  /* Media query for responsive layout on smaller screens */
  @media (max-width: 950px) {
    .container-grid {
      grid-template-columns: 1fr !important; /* Single column layout */
      gap: 25px;
    }
  }
</style>
</head>
<body>
<h1> Dashboard </h1>
<div class="container">
  <div class="container-grid">
    <div class="card" id="node1Card">
      <h2>Node 1: Temperature & Oil Level</h2>
      <div id="tempGauge" class="gauge-container"></div>
      <div id="tempValue" class="value-text">-- °C</div>
      <div>Oil Level:</div>
      <div id="oilStatus" class="badge empty">--</div>
      <div class="status-item">
        <div id="node1Led" class="led-circle"></div>
        Connection Status
      </div>
    </div>

    <div class="card" id="node2Card">
      <h2>Node 2: Door Status, Load & GPS</h2>
      <div id="doorStatus" class="value-text badge closed">--</div>
      <div id="loadGauge" class="gauge-container"></div>
      <div id="loadValue" class="value-text">--</div>
      <div class="gps-info">
        <div>Latitude: <span id="lat">--</span></div>
        <div>Longitude: <span id="lon">--</span></div>
      </div>
      <div class="status-item">
        <div id="node2Led" class="led-circle"></div>
        Connection Status
      </div>
    </div>
  </div>
</div>

<script>
  // Initialize JustGage for Temperature
  let tempGauge = new JustGage({
    id: "tempGauge",    // HTML element ID
    value: 0,           // Initial value
    min: 0,             // Minimum value
    max: 50,            // Maximum value
    title: "",          // No title needed (card heading serves this)
    label: "",          // No label needed (value-text serves this)
    decimals: 1,        // One decimal place
    gaugeWidthScale: 0.6, // Width of the gauge line
    levelColors: ["#00ff00", "#ffea00", "#ff0000"], // Green, Yellow, Red for ranges
    pointer: true,      // Show pointer
    pointerOptions: { toplength: -15, bottomlength: 10, bottomwidth: 12, color: '#8e8e93' }
  });

  // Initialize JustGage for Load
  let loadGauge = new JustGage({
    id: "loadGauge",
    value: 0,
    min: 0,
    max: 2000,
    title: "",
    label: "",
    decimals: 2,
    gaugeWidthScale: 0.6,
    levelColors: ["#ffb74d", "#f57c00", "#4caf50"], // Orange, Darker Orange, Green
    pointer: true,
    pointerOptions: { toplength: -15, bottomlength: 10, bottomwidth: 12, color: '#8e8e93' }
  });

  /**
   * @brief Sets the visual status of an LED indicator.
   * @param id The HTML element ID of the LED div.
   * @param connected Boolean indicating connection status.
   */
  function setLedStatus(id, connected) {
    const led = document.getElementById(id);
    if (connected) {
      led.classList.add('led-green'); // Add green class if connected
    } else {
      led.classList.remove('led-green'); // Remove green class if disconnected (defaults to red)
    }
  }

  /**
   * @brief Updates the UI elements with new sensor data.
   * @param data An object containing sensor values and connection statuses.
   */
  function updateUI(data) {
    // Update Temperature Gauge and Text
    tempGauge.refresh(data.temp);
    document.getElementById('tempValue').textContent = data.temp.toFixed(1) + " °C";

    // Update Oil Status Badge
    const oilStatus = document.getElementById('oilStatus');
    if (data.oilFull) {
      oilStatus.textContent = "Full";
      oilStatus.className = "badge full"; // Apply 'full' class for styling
    } else {
      oilStatus.textContent = "Empty";
      oilStatus.className = "badge empty"; // Apply 'empty' class for styling
    }

    // Update Load Gauge and Text
    loadGauge.refresh(data.load);
    document.getElementById('loadValue').textContent = data.load.toFixed(2);

    // Update Door Status Badge
    const doorStatus = document.getElementById('doorStatus');
    doorStatus.textContent = data.doorOpen ? "Open" : "Closed";
    doorStatus.className = data.doorOpen ? "value-text badge open" : "value-text badge closed";

    // Update GPS Coordinates
    document.getElementById('lat').textContent = data.lat.toFixed(6);
    document.getElementById('lon').textContent = data.lon.toFixed(6);

    // Update Node Connection Status LEDs
    setLedStatus('node1Led', data.node1Connected);
    setLedStatus('node2Led', data.node2Connected);
    // Node 3 LED is not needed as its data is merged into Node 2 card
  }

  /**
   * @brief Fetches sensor data from the ESP32 web server.
   * Calls updateUI with the fetched data.
   */
  async function fetchData() {
    try {
      const response = await fetch('/data'); // Fetch data from /data endpoint
      const data = await response.json();    // Parse JSON response
      updateUI(data);                        // Update the dashboard UI
    } catch (err) {
      console.error('Failed to fetch data:', err); // Log any errors during fetch
    }
  }

  // Initial data fetch when the page loads
  fetchData();
  // Set up interval to fetch data every 2 seconds for real-time updates
  setInterval(fetchData, 2000);
</script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html); // Send the HTML content to the client
}

/**
 * @brief Handles requests to the /data URL of the web server.
 * Sends current sensor data and connection statuses in JSON format.
 */
void handleData() {
  unsigned long now = millis();

  // Determine connection status based on last message time and timeout
  node1Connected = (now - lastNode1MsgTime) < CAN_TIMEOUT;
  node2Connected = (now - lastNode2MsgTime) < CAN_TIMEOUT;
  node3Connected = (now - lastNode3MsgTime) < CAN_TIMEOUT; // Although Node 3 data is merged, its connection status is still tracked

  // Construct JSON string with all sensor data and connection statuses
  String json = "{";
  json += "\"temp\":" + String(webTemp, 1) + ",";
  json += String("\"oilFull\":") + (webWaterEmpty ? "false" : "true") + ","; // Convert boolean to string "true" or "false"
  json += String("\"doorOpen\":") + (webDoorOpen ? "true" : "false") + ",";
  json += "\"lat\":" + String(webLatitude, 6) + ",";
  json += "\"lon\":" + String(webLongitude, 6) + ",";
  json += "\"load\":" + String(webLoad, 2) + ",";
  json += "\"node1Connected\":" + String(node1Connected ? "true" : "false") + ",";
  json += "\"node2Connected\":" + String(node2Connected ? "true" : "false") + ",";
  json += "\"node3Connected\":" + String(node3Connected ? "true" : "false"); // Include Node 3 connection status
  json += "}";
  server.send(200, "application/json", json); // Send the JSON response
}

/**
 *  Setup function, runs once when the ESP32 starts.
 * Initializes serial communication, TFT, CAN controller, WiFi, and web server.
 */
void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  SPI.begin();          // Initialize SPI bus for TFT and MCP2515

  // Initialize TFT display
  tft.initR(INITR_BLACKTAB); // Initialize ST7735 in R-type (black tab) mode
  tft.setRotation(3);        // Set display rotation (landscape)
  tft.fillScreen(ST77XX_BLACK); // Clear screen to black

  // Draw initial display elements
  drawHeader();
  drawCANStatus(false); // Initial CAN status is "NO CAN"
  drawNode1(0, false);  // Draw Node 1 section with default values
  drawNode2(false);     // Draw Node 2 section with default values

  // Initialize MCP2515 CAN controller
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Set CAN bus speed to 500KBPS with 8MHz crystal
  mcp2515.setNormalMode();                   // Set MCP2515 to normal operating mode

  Serial.println("MCP2515 Initialized and Normal Mode Set");

  // Configure LED and Buzzer pins as outputs and set initial state to LOW (off)
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  delay(3000); // Give some time for WiFi connection to stabilize before ThingSpeak init

  // Initialize ThingSpeak library with the WiFi client
  ThingSpeak.begin(client);

  // Set up web server routes
  server.on("/", handleRoot);     // Handle requests to the root URL
  server.on("/data", handleData); // Handle requests to the /data API endpoint
  server.begin();                 // Start the web server
}

/**
 * Updates the ThingSpeak channel with current sensor data.
 * Includes WiFi reconnection logic if needed.
 */
void updateThingSpeak() {
  // Check WiFi status and try to reconnect if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    // Try to reconnect for up to 10 seconds
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 10000)) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect WiFi for ThingSpeak update.");
      return; // Exit if reconnection fails
    }
  }

  // Set ThingSpeak fields with current sensor data
  ThingSpeak.setField(1, webTemp);
  ThingSpeak.setField(2, webWaterEmpty ? 0 : 1); // Field 2: Oil level (0 for Empty, 1 for Full)
  ThingSpeak.setField(3, webDoorOpen ? 1 : 0);   // Field 3: Door status (1 for Open, 0 for Closed)
  ThingSpeak.setField(4, webLatitude);
  ThingSpeak.setField(5, webLongitude);
  ThingSpeak.setField(6, webLoad);

  // Write the fields to ThingSpeak
  int response = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, thingspeakApiKey);

  // Print ThingSpeak update status to Serial Monitor
  if (response == 200) {
    Serial.println("ThingSpeak update successful");
  } else if (response == -401) {
    Serial.println("Error -401: Unauthorized. Check API key.");
  } else if (response == -304) {
    Serial.println("Error -304: Connection reset by peer.");
  } else if (response == -301) {
    Serial.println("Error -301: Connection failed, check WiFi and ThingSpeak network.");
  } else {
    Serial.print("ThingSpeak update failed, code: ");
    Serial.println(response);
  }
}

/**
 * Main loop function, runs repeatedly after setup.
 * Reads CAN messages, updates display, handles web server requests, and updates ThingSpeak.
 */
void loop() {
  struct can_frame canMsg; // Structure to hold received CAN message

  // Attempt to read a CAN message
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    lastCANMsgTime = millis(); // Update last general CAN message time

    // Update last message time for specific nodes
    if (canMsg.can_id == CAN_NODE_1_ID) {
      lastNode1MsgTime = millis();
    }
    if (canMsg.can_id == CAN_NODE_2_ID) {
      lastNode2MsgTime = millis();
    }
    if (canMsg.can_id == CAN_NODE_3_ID) {
      lastNode3MsgTime = millis();
    }

    // Process message from CAN_NODE_1_ID (Temperature and Oil Level)
    if (canMsg.can_id == CAN_NODE_1_ID && canMsg.can_dlc == 3) {
      // Extract temperature (16-bit integer, convert to float)
      int16_t tempRaw = ((int16_t)canMsg.data[0] << 8) | canMsg.data[1];
      float temperature = tempRaw / 100.0;
      // Extract oil level (1 for empty)
      bool waterEmpty = (canMsg.data[2] == 1);

      // Update TFT display only if values have changed to avoid flickering
      if (temperature != lastTemp || waterEmpty != lastWater) {
        drawNode1(temperature, waterEmpty);
        lastTemp = temperature;
        lastWater = waterEmpty;
      }

      // Control Buzzer and Red LED based on temperature
      if (temperature > 35.0) {
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(RED_LED_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(RED_LED_PIN, LOW);
      }

      // Control Yellow LED based on oil level
      if (waterEmpty) {
        digitalWrite(YELLOW_LED_PIN, HIGH);
      } else {
        digitalWrite(YELLOW_LED_PIN, LOW);
      }

      // Update web server variables
      webTemp = temperature;
      webWaterEmpty = waterEmpty;

    }
    // Process message from CAN_NODE_2_ID (Door Status and Load)
    else if (canMsg.can_id == CAN_NODE_2_ID && canMsg.can_dlc >= 5) {
      // Extract door status (1 for open)
      bool doorOpen = (canMsg.data[0] == 1);
      float loadWeight;
      // Copy 4 bytes of data directly into a float variable
      memcpy(&loadWeight, &canMsg.data[1], sizeof(float));

      // Update TFT display only if door status has changed
      if (doorOpen != lastDoor) {
        drawNode2(doorOpen);
        lastDoor = doorOpen;
      }

      // Update web server variables
      node2Load = loadWeight; // Also update the global variable used by drawNode2
      webDoorOpen = doorOpen;
      webLoad = loadWeight;

      // Control Green LED based on door status
      if (doorOpen) {
        digitalWrite(GREEN_LED_PIN, LOW); // Green LED off if door open
      } else {
        digitalWrite(GREEN_LED_PIN, HIGH); // Green LED on if door closed
      }

    }
    // Process message from CAN_NODE_3_ID (GPS Coordinates)
    else if (canMsg.can_id == CAN_NODE_3_ID && canMsg.can_dlc >= 8) {
      float lat_val, lon_val;
      uint8_t latBytes[4], lonBytes[4];
      // Copy 4 bytes for latitude and 4 bytes for longitude
      for (int i = 0; i < 4; i++) {
        latBytes[i] = canMsg.data[i];
        lonBytes[i] = canMsg.data[i + 4];
      }
      // Copy byte arrays into float variables
      memcpy(&lat_val, latBytes, sizeof(float));
      memcpy(&lon_val, lonBytes, sizeof(float));

      // Update global variables for TFT display and web server
      node3Latitude = lat_val;
      node3Longitude = lon_val;

      webLatitude = lat_val;
      webLongitude = lon_val;
    }
  }

  // Always update CAN status on TFT based on overall CAN message timeout
  drawCANStatus((millis() - lastCANMsgTime) < CAN_TIMEOUT);
  // Redraw Node 2 section to ensure latest GPS data is displayed, even if no new CAN message from Node 2 or 3
  drawNode2(lastDoor);

  // Handle incoming web server requests
  server.handleClient();

  // ThingSpeak periodic update logic
  unsigned long now = millis();
  if (now - lastThingSpeakUpdate >= THINGSPEAK_UPDATE_INTERVAL) {
    updateThingSpeak();      // Call the ThingSpeak update function
    lastThingSpeakUpdate = now; // Reset the timer
  }

  delay(200); // Small delay to prevent busy-waiting and allow other tasks to run
}