# LifeLink Smartwatch - ESP32-S3

[🇷🇸 Srpski verzija dokumentacije nalazi se u README_RS.md](README_RS.md)

LifeLink is an advanced smartwatch prototype built on the **ESP32-S3** platform, utilizing **ESP-IDF** alongside **LVGL** for a rich graphical interface (466x466 AMOLED display). It focuses on elderly care, health tracking, and robust emergency response functionality.

## Features

- **Advanced Fall Detection**: Utilizes the QMI8658 IMU (Accelerometer + Gyroscope) to detect sudden drops (Free Fall) and hard impacts. It requires extended stillness after an impact coupled with an orientation shift to confirm a real fall and avoid false alarms.
- **Improved MAX30102 Algorithm**: Hand-tuned heart rate and SpO2 calculation using FFT on a 100Hz sampling window. 
  - **Artifact Rejection**: Filters out low-frequency noise (below 45 BPM) and physiologically impossible values (above 220 BPM).
  - **Reflective Optimization**: Calibrated SpO2 formula (`104 - 17*R`) specifically for wrist-based reflective sensing.
- **Autonomous Emergency Response**: The watch can operate fully independently of the mobile app in an emergency:
  - **Sequential SMS**: Sends custom SOS messages to multiple emergency contacts.
  - **Sequential Calls**: Automatically initiates voice calls to primary contacts if a fall is confirmed.
  - **SOS Call**: One-button or automatic priority emergency dispatch.
- **I2C Fast Mode (400kHz)**: Optimized I2C bus clock speed for ultra-reliable communication across all shared sensors (MAX30102, QMI8658, AXP2101).
- **Automated GSM Emergency SMS**: Communicates with a SIM800L/A6 GSM Module to send background SMS alerts containing:
  - Precise GPS coordinates formatted as a direct Google Maps link.
  - Real-time heart rate at the time of the event.
  - Contextual warnings stating whether the fall was real or simulated.
- **WiFi Cloud Connectivity**: Directly connects to WiFi and uploads health metrics and GPS coordinates to Firestore REST API every 30 seconds for remote dashboard monitoring.
- **Always-On Display (AOD)**: Energy-efficient idle mode showing time on a dimmed black background, extending battery life while maintaining utility.
- **NMEA GPS Support**: Integrated parsing of NMEA strings from external GPS modules for precise location tracking.

## Companion Mobile App (Flutter)

A cross-platform **Flutter** companion app extends LifeLink's capabilities via Bluetooth Low Energy (BLE):

- **Real-Time Dashboard**: Live Heart Rate (BPM), SpO2, G-Force, and GPS location mirrored from the watch.
- **BLE Connectivity**: Automatic or manual pairing with the LifeLink ESP32 wearable via BLE SPP.
- **Emergency Response**: Configurable fall response actions — direct phone **Call**, **SMS** with GPS coordinates, or system-wide **SOS** intent.
- **Fall Mirroring**: App mirrors the watch's 3-stage fall detection (Safe → Warning → Alarm) with a 5-second countdown and haptic/audio alerts.
- **Settings**: Configure emergency contact, fall action type, countdown duration, and default device MAC address.
- **Interactive Map**: Displays the user's location on an OpenStreetMap view for responder assistance.

## Hardware Stack

- **MCU**: ESP32-S3
- **Display**: Round AMOLED (466 x 466)
- **Cellular**: SIM800L GSM Module (Communicating via AT Commands over UART, powered directly from 3.7V Li-Ion battery)
- **IMU Sensor**: QMI8658 (Accelerometer & Gyroscope)
- **Health Sensor**: MAX30102 (HR & SpO2)
- **Power Management**: AXP2101

## Setup & Building

This project is built using the official [Espressif ESP-IDF framework](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/) (v5.x recommended).

### 1. Configure the Project
Make sure your target is properly mapped to the ESP32-S3:
```bash
idf.py set-target esp32s3
idf.py menuconfig
```
### 2. Build and Flash
Compile the source code and flash it directly to your connected device:
```bash
idf.py build
idf.py -p COM_PORT flash monitor
```
### 3. WiFi & Firestore Config
Set your WiFi credentials in `lifelink.cpp` or via the mobile app. Ensure `YOUR_PROJECT_ID` is updated in `lifelink.cpp` for Firestore REST API access.

## Application Flow

1. **Dashboard (Screen 1)**: Primary clock face, health vitals, and connectivity status icons.
2. **Fall Trigger/Debug (Screen 2)**: Simulate a fall, or engage the raw hardware debug sensors stream.
3. **Settings (Screen 3)**: Utilize the customized numpad to input or replace your dedicated emergency contact's GSM phone number.
4. **Emergency Countdown (Screen 4)**: Activated upon Fall Confirmation. Provides an unmissable red-alert interface allowing users to abort the alert before it attempts to connect via the SIM800L modem to send an SMS.
