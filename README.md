
# 🚛 Real-Time Engine Performance & Vehicle Load Monitoring System Using CAN Protocol

A comprehensive system for **real-time data acquisition and analysis** of engine performance and vehicle load in heavy vehicles, leveraging the **Controller Area Network (CAN) protocol**. This system continuously monitors critical vehicle parameters, ensuring optimized operation, safety, and proactive maintenance.

<img width="1401" height="415" alt="Problem" src="https://github.com/user-attachments/assets/e771b57c-4916-410b-b4cb-db5d9df92078" />


## 📋 Overview

This project designs and implements a robust embedded system that acquires data from various sensors connected over the CAN bus, processes it in real-time, and provides actionable insights via onboard displays and remote fleet management platforms.

<img width="1266" height="757" alt="1_Block_Diagram" src="https://github.com/user-attachments/assets/b53324b8-89d0-40f2-b118-6e109c941168" />

## 📋 Block Diagram

   ![Block_Diagram](https://github.com/user-attachments/assets/c7bf307c-c6b5-4a83-8241-b745e3180d4c)

---

## ⚙️ Key Functionalities

### 1. 🔥 Real-Time Engine Performance Monitoring
- Continuous acquisition of engine parameters:  
  - Engine temperature  
  - Fuel consumption  
  - Oil Level
- Data processing to evaluate engine health and efficiency.

### 2. ⚖️ Vehicle Load Measurement and Analysis
- Load sensors/strain gauges measure chassis weight/stress.  
- Load data transmitted via CAN to the central ECU.  
- Helps prevent overloading and enhances fuel efficiency.

### 3. 🔌 Connection & Condition Monitoring
- Continuous monitoring of sensor and electrical connection status.  
- Fault detection for loose connections, sensor failures, or wiring damage.  
- Diagnostic Trouble Codes (DTCs) read and transmitted for maintenance.

### 4. ⚠️ Damage & Fault Alerts
- Alarms/warnings displayed on dashboard and sent via GSM to fleet managers.  
- Enables proactive maintenance, minimizing downtime.

### 6. 📡 Communication & Data Display
- CAN frames transmit sensor data between nodes.  
- Master ECU aggregates and displays data on LCD/digital dashboard.  
- CAN bus ensures reliable, high-speed communication with error detection.

### 7. 🌐 Remote Monitoring & Fleet Management Integration
- Telematics module sends CAN data to cloud/fleet management software.  
- Real-time vehicle health, driver behavior, fuel efficiency, and compliance insights.  
- Remote diagnostics reduce physical inspections and improve efficiency.


## 🧰 Software Tools & Technologies

- **Embedded C/C++** for microcontroller firmware development.  
- **CAN Protocol Stack** libraries (mcp2515.h CAN).  
- **Real-Time Operating System (RTOS).
- **Data Analysis Algorithms** implemented on ECU or cloud platform (Thingspeak). 
- **GSM Module Firmware** for communication protocols (GPRS, HTTP).  


### Step 1: Hardware Setup
- RISC (ESP32) microcontroller with MCP2515 CAN Module.
- Interface sensors (temperature, load, Oil Level, Door Status) to MCU. 
- Set up display unit (1.8 inch TFT LCD Module) connected via SPI. 
- Integrate GSM module for remote communication.

### Step 2: Firmware Development
- Initialize CAN controller and configure baud rate (typically 500 kbps).  
- Develop sensor data acquisition routines with proper sampling rates.  
- Implement CAN message formatting and transmission for sensor data. 
- Write algorithms for engine health temp, load analysis. 
- Program dashboard display updates with real-time data and alerts.  
- Integrate GSM module communication routines for remote data upload. 

### Step 3: Testing & Calibration
- Calibrate sensors for accurate measurement (temperature, load, oil level, door status).  
- Test CAN communication reliability and error handling.  
- Validate real-time data processing and alert generation.  
- Simulate fault conditions to verify diagnostics and alarms.  

### Step 4: Deployment & Maintenance
- Install system in heavy vehicle with secure sensor mounting and wiring.  
- Monitor system performance during operation.  
- Use fleet management software for continuous vehicle health insights.  
- Perform maintenance based on alerts and diagnostic codes.  

---

## 🚀 Benefits of the System

- Enhanced vehicle safety through early fault detection. 
- Optimized fuel consumption and load management.  
- Reduced downtime with proactive maintenance alerts.  
- Improved fleet management with real-time remote monitoring.  
- Reliable, high-speed communication via CAN protocol.  
- Fleet managers receive real-time insights into vehicle health, driver behavior, fuel efficiency, and compliance.
- Remote diagnostics reduce the need for physical inspections and improve operational efficiency.
  
![7_Working_photo](https://github.com/user-attachments/assets/0a4e40ea-c534-47cd-9d51-c2ed9c47ac1f)

---

# Bill (Indian Rupees - INR)

| Component                   | Quantity | Unit Cost (INR) | Total Cost (INR) |
|----------------------------|----------|-----------------|------------------|
| ESP32 Module               | 3        | 400             | 1,200            |
| 16x2 LCD Display           | 1        | 150             | 150              |
| MAX6675 (Temp Sensor)      | 1        | 390             | 390              |
| Float Sensor               | 1        | 300             | 300              |
| CAN Module                 | 3        | 280             | 840              |
| Load Sensor                | 1        | 350             | 350              |
| Door Sensor                | 1        | 140             | 140              |
| OLED Display               | 1        | 230             | 230              |
| TFT ST7735 Display 1.5 inch| 1        | 450             | 450              |
| GSM Module                 | 1        | 640             | 640              |
| Miscellaneous (wires, PCB, connectors) | 1 | 500          | 500              |
| **Total Estimated Cost**   |     15     |                 | **5,130**        |

---

## 🚀 Notes 

---

## 1.8 inch TFT LCD Display
 
- **SPI Advantages over I2C for TFT:**
  - Supports high-speed communication (tens of MHz) versus I2C's max around 400 kHz to 1 MHz.
  - Enables fast screen refresh necessary for smooth graphics.
  - Full-duplex communication and separate data lines allow efficient data transfer.
  - Simpler timing protocol without I2C start/stop conditions or clock stretching.
- **Reasons for Choosing TFT over OLED:**
  - 128x160 pixels, 18-bit color depth (262,144 colors) 
  - Lower cost and better availability.
  - Greater resistance to screen burn-in and longer lifespan.
  - Higher brightness and better visibility in sunlight.
  - Compatible with common microcontrollers due to onboard driver chips (e.g., ST7735).
  - Stable color performance over long-term use.

---

## MCP2515 CAN Bus Module

- **Purpose:** Add CAN bus capability to microcontrollers via SPI interface.  
- **CAN Bus Support:** Up to 112 nodes on the bus.  
- **Data Rate:** Up to 1 Mbps, compliant with ISO-11898 standard.  
- **Supported Frames:**
  - Standard frames (11-bit identifier).
  - Extended frames (29-bit identifier).
  - Both data frames and remote frames with payload from 0 to 8 bytes.  
- **Clock Source:** 8 MHz external crystal oscillator with programmable prescaler and clock output pin.  

### Components and Functionality:

- **MCP2515 Controller:**  
  - Implements CAN protocol handling: message preparation, transmission, reception, filtering, and error checking.  
  - Manages access to CAN bus.  
  - Sends interrupts to MCU on events (message received/transmitted/errors).

- **TJA1050 Transceiver:**  
  - Handles physical signaling on the CAN bus wires (CAN_H and CAN_L).  
  - Converts logic signals from MCP2515 into differential CAN signals and vice versa.
 
    ---
