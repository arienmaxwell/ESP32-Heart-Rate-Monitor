# ESP32 Heart Rate Monitoring System
ESP32-based Heart Rate Monitor with OLED display and real-time web interface (STA &amp; AP modes)
This repository contains multiple ESP32-based heart rate monitoring projects using pulse sensors, OLED displays, and WiFi connectivity. It’s designed for health tracking applications, including remote monitoring and on-device display.

## 🧠 Overview

This project explores different modes of heart rate monitoring using the ESP32 microcontroller:
- Local display with OLED
- WiFi Access Point mode for wireless monitoring
- Data acquisition for signal analysis
- Web-based monitoring via a built-in web server

## 🛠️ Features

- Real-time heart rate monitoring
- Pulse Sensor interfacing
- OLED Display for visual feedback
- Web server interface for remote monitoring
- Multiple test configurations for debugging and experimentation

## 🔌 Hardware Requirements

- ESP32 Development Board
- Pulse Sensor (e.g., Pulse Sensor Amped)
- SSD1306 OLED Display (I2C)
- Jumper wires and breadboard
- (Optional) Push button for manual control

## 📦 Libraries Used

Make sure to install the following libraries via the Arduino Library Manager:

- `Adafruit SSD1306`
- `Adafruit GFX`
- `ESPAsyncWebServer`
- `WiFi` (built-in for ESP32)
- `PulseSensorPlayground` (if used in any sketch)

## 🚀 Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/arienmaxwell/ESP32-Heart-Rate-Monitor.git


