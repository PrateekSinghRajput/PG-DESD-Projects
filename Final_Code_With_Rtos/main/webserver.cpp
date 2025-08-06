#include "webserver.h"
#include <WebServer.h>
#include <Arduino.h>
#include "globals.h"

static WebServer* _server = nullptr;

void handleRoot();
void handleData();

void setupWebServer(WebServer &server) {
  _server = &server;
  _server->on("/", handleRoot);
  _server->on("/data", handleData);
}

void handleRoot() {
  const char* html = R"rawliteral(
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

void handleData() {
  unsigned long now = millis();

  node1Connected = (now - lastNode1MsgTime) < CAN_TIMEOUT;
  node2Connected = (now - lastNode2MsgTime) < CAN_TIMEOUT;
  node3Connected = (now - lastNode3MsgTime) < CAN_TIMEOUT;

  String json = "{";
  json += "\"temp\":" + String(webTemp, 1) + ",";
  json += "\"oilFull\":" + String(webWaterEmpty ? "false" : "true") + ",";
  json += "\"doorOpen\":" + String(webDoorOpen ? "true" : "false") + ",";
  json += "\"lat\":" + String(webLatitude, 6) + ",";
  json += "\"lon\":" + String(webLongitude, 6) + ",";
  json += "\"load\":" + String(webLoad, 2) + ",";
  json += "\"node1Connected\":" + String(node1Connected ? "true" : "false") + ",";
  json += "\"node2Connected\":" + String(node2Connected ? "true" : "false") + ",";
  json += "\"node3Connected\":" + String(node3Connected ? "true" : "false");
  json += "}";
  _server->send(200, "application/json", json);
}
