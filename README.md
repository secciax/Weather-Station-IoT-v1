# IoT Weather Station (BLE, WiFi & MQTT)

IoT weather station prototype for atmospheric data collection and analysis.

## 📋 Project Description

This project implements a two-module weather station based on the Arduino architecture. The two nodes communicate with low power consumption via the Bluetooth Low Energy (BLE) protocol. Data is then routed to an MQTT broker (ThingSpeak) for remote monitoring.

This project represents "Phase 1" of a Bachelor's thesis in Computer Engineering, with the future goal of integrating LoRaWAN connectivity and Machine Learning models (Edge AI) for local weather forecasting.

## 🏗️ System Architecture

The system consists of two main nodes:

### 1. Outdoor Module (Sensor Node / BLE Peripheral)
Placed outdoors, it handles physical data acquisition.
* **Hardware:** Arduino Nano 33 BLE Sense Rev2
* **Sensors:** Temperature, Humidity, Pressure (integrated on-board)
* **Operational Logic:**
  1. Wakes up from Deep Sleep.
  2. Acquires sensor data.
  3. Updates the characteristics of the `servizioMeteo` BLE service (`NOTIFY` mode).
  4. Returns to power-saving mode by shutting down the CPU.

### 2. Indoor Module (Gateway / BLE Central)
Placed indoors, it acts as a bridge between the PAN (BLE) and the WAN (WiFi/MQTT).
* **Hardware:** Arduino Nano RP2040 Connect
* **Display:** 16x2 LCD Screen (I2C)
* **Operational Logic:**
  1. Scans for incoming BLE advertising packets.
  2. Subscribes to the outdoor module's `NOTIFY` service.
  3. Upon receiving data (every 10 minutes), updates the local LCD display.
  4. Publishes the payload to the MQTT broker (ThingSpeak).

## ⚙️ Installation and Configuration

To replicate this project, clone the repository and configure your network credentials:

1. Open the `Modulo_Interno` folder.
2. Locate the file named `secrets_template.h`.
3. Rename it to `secrets.h`.
4. Enter your actual credentials (WiFi SSID, Password, and MQTT credentials):

```cpp
#define SECRET_SSID "YOUR_WIFI_SSID"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
#define SECRET_MQTT_CLIENT_ID "YOUR_CLIENT_ID"
#define SECRET_SSID "IL_TUO_WIFI"
#define SECRET_PASS "LA_TUA_PASSWORD"
#define SECRET_MQTT_CLIENT_ID "IL_TUO_CLIENT_ID"
// ...
