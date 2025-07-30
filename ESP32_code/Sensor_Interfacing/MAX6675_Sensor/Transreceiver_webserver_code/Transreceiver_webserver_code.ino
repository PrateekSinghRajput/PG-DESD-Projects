#include <SPI.h>
#include <mcp2515.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <WebServer.h>

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
unsigned long lastNode3MsgTime = 0;

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

const char* ssid = "Prateek";
const char* password = "justdoelectronics@#12345";

WebServer server(80);

float webTemp = 0;
bool webWaterEmpty = false;
bool webDoorOpen = false;
float webLatitude = 12.971598;
float webLongitude = 77.594566;
float webLoad = 0.0;

bool node1Connected = false;
bool node2Connected = false;
bool node3Connected = false;

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

void drawNode3(float latitude, float longitude, float load) {
  tft.fillRect(0, 102, 160, 58, ST77XX_BLACK);
  tft.drawRect(0, 102, 160, 58, ST77XX_WHITE);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 108);
  tft.print("Node3");
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  tft.setCursor(70, 108);
  tft.print("Load: ");
  tft.print(load, 2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(6, 118);
  tft.print(latitude, 6);
  tft.setCursor(70, 118);
  tft.print(longitude, 6);
}

void handleRoot() {
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
  @import url('https://fonts.googleapis.com/css2?family=Roboto&display=swap');

  * {
    box-sizing: border-box;
  }

  body {
    font-family: 'Roboto', Arial, sans-serif;
    background: #1e1e2f;
    color: #eee;
    margin: 0;
    padding: 20px;
    text-align: center;
    -webkit-font-smoothing: antialiased;
  }

  h1 {
    color: #4fc3f7;
    margin-bottom: 30px;
    text-shadow: 0 0 10px #4fc3f7;
    font-weight: 700;
    font-size: 2.5rem;
  }

  .container {
    max-width: 960px;
    margin: 0 auto;
    padding: 0 15px;
  }

  /* Single row with 3 evenly spaced columns */
  .container-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 30px;
  }

  .card {
    background: #2c2c3f;
    border-radius: 15px;
    padding: 25px 20px;
    box-shadow: 0 0 20px rgba(0,0,0,0.9);
    color: #eee;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 340px;
    transition: transform 0.3s ease;
  }
  .card:hover {
    transform: translateY(-8px);
    box-shadow: 0 0 35px #4fc3f7;
  }

  .card h2 {
    margin-top: 0;
    margin-bottom: 20px;
    color: #4fc3f7;
    font-weight: 700;
    font-size: 1.8rem;
  }

  .gauge-container {
    width: 200px;
    height: 160px;
    margin: 0 auto 1rem auto;
  }

  .value-text {
    font-size: 1.8rem;
    font-weight: 600;
    margin: 10px 0 18px 0;
  }

  .badge {
    font-size: 1.2rem;
    font-weight: 700;
    padding: 6px 24px;
    border-radius: 25px;
    color: white;
    user-select: none;
    box-shadow: 0 0 8px rgba(0,0,0,0.8);
    margin: 10px 0;
  }
  .badge.full {
    background-color: #4caf50;
    box-shadow: 0 0 20px #4caf5080;
  }
  .badge.empty {
    background-color: #f44336;
    box-shadow: 0 0 20px #f4433680;
  }
  .badge.open {
    background-color: #43a047;
    box-shadow: 0 0 20px #43a047cc;
  }
  .badge.closed {
    background-color: #e53935;
    box-shadow: 0 0 20px #e5393577;
  }

  .led-circle {
    width: 22px;
    height: 22px;
    border-radius: 50%;
    background: red;
    box-shadow: 0 0 12px red;
    margin-right: 12px;
    display: inline-block;
    vertical-align: middle;
    transition: background-color 0.3s ease, box-shadow 0.3s ease;
  }
  .led-green {
    background: #4caf50;
    box-shadow: 0 0 15px #4caf50;
  }

  .status-item {
    font-size: 1.3rem;
    color: #eee;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 12px;
    margin-top: 15px;
  }

  .gps-info {
    margin-top: 15px;
    font-size: 1.3rem;
    text-align: left;
    max-width: 260px;
  }

  @media (max-width: 950px) {
    /* Stack cards vertically on smaller screens */
    .container-grid {
      grid-template-columns: 1fr !important;
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
      <h2>Node 2: Door Status</h2>
      <div id="doorStatus" class="value-text badge closed">--</div>
      <div class="status-item">
        <div id="node2Led" class="led-circle"></div>
        Connection Status
      </div>
    </div>

    <div class="card" id="node3Card">
      <h2>Node 3: GPS & Load</h2>
      <div id="loadGauge" class="gauge-container"></div>
      <div id="loadValue" class="value-text">--</div>
      <div class="gps-info">
        <div>Latitude: <span id="lat">--</span></div>
        <div>Longitude: <span id="lon">--</span></div>
      </div>
      <div class="status-item">
        <div id="node3Led" class="led-circle"></div>
        Connection Status
      </div>
    </div>
  </div>
</div>

<script>
  let tempGauge = new JustGage({
    id: "tempGauge",
    value: 0,
    min: 0,
    max: 50,
    title: "",
    label: "",
    decimals: 1,
    gaugeWidthScale: 0.6,
    levelColors: ["#00ff00", "#ffea00", "#ff0000"],
    pointer: true,
    pointerOptions: { toplength: -15, bottomlength: 10, bottomwidth: 12, color: '#8e8e93' }
  });

  let loadGauge = new JustGage({
    id: "loadGauge",
    value: 0,
    min: 0,
    max: 100,
    title: "",
    label: "",
    decimals: 2,
    gaugeWidthScale: 0.6,
    levelColors: ["#ffb74d", "#f57c00", "#4caf50"],
    pointer: true,
    pointerOptions: { toplength: -15, bottomlength: 10, bottomwidth: 12, color: '#8e8e93' }
  });

  function setLedStatus(id, connected) {
    const led = document.getElementById(id);
    if (connected) {
      led.classList.add('led-green');
    } else {
      led.classList.remove('led-green');
    }
  }

  function updateUI(data) {
    tempGauge.refresh(data.temp);
    document.getElementById('tempValue').textContent = data.temp.toFixed(1) + " °C";

    const oilStatus = document.getElementById('oilStatus');
    if (data.oilFull) {
      oilStatus.textContent = "Full";
      oilStatus.className = "badge full";
    } else {
      oilStatus.textContent = "Empty";
      oilStatus.className = "badge empty";
    }

    loadGauge.refresh(data.load);
    document.getElementById('loadValue').textContent = data.load.toFixed(2);

    const doorStatus = document.getElementById('doorStatus');
    doorStatus.textContent = data.doorOpen ? "Open" : "Closed";
    doorStatus.className = data.doorOpen ? "value-text badge open" : "value-text badge closed";

    document.getElementById('lat').textContent = data.lat.toFixed(6);
    document.getElementById('lon').textContent = data.lon.toFixed(6);

    setLedStatus('node1Led', data.node1Connected);
    setLedStatus('node2Led', data.node2Connected);
    setLedStatus('node3Led', data.node3Connected);
  }

  async function fetchData() {
    try {
      const response = await fetch('/data');
      const data = await response.json();
      updateUI(data);
    } catch (err) {
      console.error('Failed to fetch data:', err);
    }
  }

  fetchData();
  setInterval(fetchData, 2000);
</script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleData() {
  unsigned long now = millis();

  node1Connected = (now - lastNode1MsgTime) < CAN_TIMEOUT;
  node2Connected = (now - lastNode2MsgTime) < CAN_TIMEOUT;
  node3Connected = (now - lastNode3MsgTime) < CAN_TIMEOUT;

  String json = "{";
  json += "\"temp\":" + String(webTemp, 1) + ",";
  json += String("\"oilFull\":") + (webWaterEmpty ? "false" : "true") + ",";
  json += String("\"doorOpen\":") + (webDoorOpen ? "true" : "false") + ",";
  json += "\"lat\":" + String(webLatitude, 6) + ",";
  json += "\"lon\":" + String(webLongitude, 6) + ",";
  json += "\"load\":" + String(webLoad, 2) + ",";
  json += "\"node1Connected\":" + String(node1Connected ? "true" : "false") + ",";
  json += "\"node2Connected\":" + String(node2Connected ? "true" : "false") + ",";
  json += "\"node3Connected\":" + String(node3Connected ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
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

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

// -------- Loop --------
void loop() {
  struct can_frame canMsg;

  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    lastCANMsgTime = millis();

    if (canMsg.can_id == CAN_NODE_1_ID) {
      lastNode1MsgTime = millis();
    }
    if (canMsg.can_id == CAN_NODE_2_ID) {
      lastNode2MsgTime = millis();
    }
    if (canMsg.can_id == CAN_NODE_3_ID) {
      lastNode3MsgTime = millis();
    }

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

      webTemp = temperature;
      webWaterEmpty = waterEmpty;

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

      webDoorOpen = doorOpen;

    } else if (canMsg.can_id == CAN_NODE_3_ID && canMsg.can_dlc >= 8) {
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

      webLatitude = lat_val;
      webLongitude = lon_val;
    }
  }

  drawCANStatus((millis() - lastCANMsgTime) < CAN_TIMEOUT);
  drawNode3(node3Latitude, node3Longitude, node3Load);

  server.handleClient();

  delay(200);
}
