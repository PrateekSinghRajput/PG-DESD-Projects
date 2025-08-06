
# 🚛 Real-Time Engine Performance & Vehicle Load Monitoring System Using CAN Protocol

A comprehensive system for **real-time data acquisition and analysis** of engine performance and vehicle load in heavy vehicles, leveraging the **Controller Area Network (CAN) protocol**. This system continuously monitors critical vehicle parameters, ensuring optimized operation, safety, and proactive maintenance.

<img width="1401" height="415" alt="Problem" src="https://github.com/user-attachments/assets/e771b57c-4916-410b-b4cb-db5d9df92078" />


## 📋 Overview

This project designs and implements a robust embedded system that acquires data from various sensors connected over the CAN bus, processes it in real-time, and provides actionable insights via onboard displays and remote fleet management platforms.

<img width="1266" height="757" alt="1_Block_Diagram" src="https://github.com/user-attachments/assets/b53324b8-89d0-40f2-b118-6e109c941168" />


---

## ⚙️ Key Functionalities

### 1. 🔥 Real-Time Engine Performance Monitoring
- Continuous acquisition of engine parameters:  
  - Engine temperature  
  - RPM (Revolutions Per Minute)  
  - Fuel consumption  
  - Throttle position  
  - Oil pressure  
- Data processing to evaluate engine health and efficiency.

### 2. ⚖️ Vehicle Load Measurement and Analysis
- Load sensors/strain gauges measure chassis weight/stress.  
- Load data transmitted via CAN to the central ECU.  
- Helps prevent overloading and enhances fuel efficiency.

### 3. 🛠 Leak Detection & Pneumatic System Monitoring
- Pressure sensors monitor pneumatic brake and suspension circuits.  
- Algorithms detect pressure deviations indicating leaks or faults.  
- Early leak detection improves safety and reduces damage costs.

### 4. 🔌 Connection & Condition Monitoring
- Continuous monitoring of sensor and electrical connection status.  
- Fault detection for loose connections, sensor failures, or wiring damage.  
- Diagnostic Trouble Codes (DTCs) read and transmitted for maintenance.

### 5. ⚠️ Damage & Fault Alerts
- Vibration sensors detect mechanical damage or abnormal vibrations.  
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

- **Embedded C/C++** for microcontroller firmware development  
- **CAN Protocol Stack** libraries (STM32 HAL CAN)  
- **Real-Time Operating System (RTOS)** (optional for task scheduling)  
- **Data Analysis Algorithms** implemented on ECU or cloud platform   
- **GSM Module Firmware** for communication protocols (GPRS, MQTT)  


### Step 1: Hardware Setup
- Select a microcontroller with built-in CAN controller (e.g., STM32, PIC18F)  
- Connect CAN transceiver (e.g., MCP2551) to microcontroller CAN pins  
- Interface sensors (temperature, RPM, load, pressure, vibration) to MCU ADC or digital inputs  
- Set up display unit (LCD/OLED) connected via SPI/I2C or parallel interface  
- Integrate GSM/telematics module for remote communication  

### Step 2: Firmware Development
- Initialize CAN controller and configure baud rate (typically 500 kbps)  
- Develop sensor data acquisition routines with proper sampling rates  
- Implement CAN message formatting and transmission for sensor data  
- Write algorithms for engine health assessment, load analysis, and leak detection  
- Implement fault detection and DTC generation  
- Program dashboard display updates with real-time data and alerts  
- Integrate GSM module communication routines for remote data upload  

### Step 3: Testing & Calibration
- Calibrate sensors for accurate measurement (temperature, load, pressure)  
- Test CAN communication reliability and error handling  
- Validate real-time data processing and alert generation  
- Simulate fault conditions to verify diagnostics and alarms  
- Test remote data transmission and fleet management integration  

### Step 4: Deployment & Maintenance
- Install system in heavy vehicle with secure sensor mounting and wiring  
- Monitor system performance during operation  
- Use fleet management software for continuous vehicle health insights  
- Perform maintenance based on alerts and diagnostic codes  

---

## 🚀 Benefits of the System

- Enhanced vehicle safety through early fault detection  
- Optimized fuel consumption and load management  
- Reduced downtime with proactive maintenance alerts  
- Improved fleet management with real-time remote monitoring  
- Reliable, high-speed communication via CAN protocol  
- Fleet managers receive real-time insights into vehicle health, driver behavior, fuel efficiency, and compliance.
- Remote diagnostics reduce the need for physical inspections and improve operational efficiency.
