# Alerting System for Structural Durability 

A smart IoT-based system to monitor the structural health of buildings using an ESP32, multiple environmental and motion sensors, OLED display, Blynk cloud dashboard, and automated email alerts.

---

## 📦 Features

- 📐 **Load Monitoring** using HX711 load cell amplifier
- 🌡️ **Temperature & Humidity** via DHT11 sensor
- 📳 **Vibration Detection** with SW420 sensor and interrupt-based counting
- 📠 **Tilt & Inclination Sensing** via SW18010P (digital + analog with reversed angle calculation)
- 📺 **Local Display** with OLED SSD1306 (rotating pages every 3 seconds)
- ☁️ **Cloud Monitoring** with Blynk IoT platform and real-time data sync
- 📬 **Email Alerts** using ESP_Mail_Client for critical threshold breaches
- 📊 **Durability Scoring** with automated periodic assessment reports
- 🧠 **Custom Thresholds** and remote calibration via mobile app
- 🔄 **Auto-Reset Functions** and cooldown protection for alerts

---

## 🧰 Hardware Components

| Component           | Model           | Purpose                     | Notes |
|---------------------|-----------------|-----------------------------|-------|
| Microcontroller     | ESP32           | WiFi-enabled control unit   | Main processor |
| Load Cell Module    | HX711 + Load Cell| Weight/load sensing        | 24-bit ADC |
| Temperature Sensor  | DHT11           | Temp + Humidity monitoring  | Digital output |
| Vibration Sensor    | SW420           | Motion/vibration detection  | Digital + analog |
| Tilt Sensor         | SW18010P        | Tilt & angle sensing        | Dual output mode |
| Display             | OLED SSD1306    | Local data display (128x32) | I2C interface |

---

## ⚡ Pin Configuration

### 📌 Wiring Summary

| ESP32 Pin | Sensor Pin / Purpose | Component          | Signal Type |
|-----------|----------------------|--------------------|-------------|
| GPIO 25   | DOUT                 | HX711 (Load Cell)  | Digital     |
| GPIO 26   | SCK                  | HX711              | Clock       |
| GPIO 27   | DATA                 | DHT11              | Digital     |
| GPIO 34   | Digital Out          | SW420 Vibration    | Digital     |
| GPIO 14   | Digital Out          | SW18010P Tilt      | Digital     |
| GPIO 32   | Analog Out           | SW18010P Tilt      | Analog      |
| GPIO 21   | I2C SDA              | OLED Display       | I2C Data    |
| GPIO 22   | I2C SCL              | OLED Display       | I2C Clock   |
| 3.3V / 5V | Power Input          | All Sensors        | Power       |
| GND       | Ground               | All Sensors        | Ground      |

---

## 📲 Blynk Integration

### Virtual Pin Mapping

| Pin | Function              | Widget Type     | Range/Options |
|-----|-----------------------|-----------------|---------------|
| V0  | Weight Display        | Gauge           | 0-1000g       |
| V1  | Temperature           | Gauge           | 0-50°C        |
| V2  | Humidity              | Gauge           | 0-100%        |
| V3  | Vibration Status      | LED             | ON/OFF        |
| V4  | Vibration Count       | Value Display   | 0-999999      |
| V5  | Tilt Status           | LED             | ON/OFF        |
| V6  | Tilt Angle            | Gauge           | 0-180°        |
| V7  | Tilt Duration         | Value Display   | Seconds       |
| V8  | System Status         | Value Display   | Text Status   |
| V9  | Log/Notifications     | Terminal        | Event Log     |
| V10 | Reset Vibration       | Button          | Momentary     |
| V11 | Calibrate Load Cell   | Button          | Momentary     |
| V12 | Vibration Threshold   | Slider          | 0-1000        |
| V13 | Tilt Angle Threshold  | Slider          | 0-90°         |
| V14 | Durability Status     | Value Display   | Text Status   |

### 📱 Blynk Events Configuration
Configure these events in your Blynk console:
- `vibration_alert` - Triggered when vibration threshold exceeded
- `tilt_alert` - Triggered when critical tilt detected
- `durability_alert` - Triggered for critical structural conditions
- `structural_alert` - General structural integrity alerts

---

## 📧 Email Alerts Setup

### 🔐 Gmail Configuration
For Gmail SMTP, you need to:
1. Enable 2-Factor Authentication
2. Generate an App Password
3. Use the App Password in the code

```cpp
// Email credentials - UPDATE THESE
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "your-email@gmail.com"
#define AUTHOR_PASSWORD "your-16-char-app-password"
#define RECIPIENT_EMAIL "receiver@gmail.com"
```

### 📨 Alert Types
- **Vibration Alert** - Sent when vibration count exceeds threshold
- **Tilt Alert** - Sent when tilt angle and duration exceed limits  
- **Durability Report** - Periodic structural health assessment (every 5 minutes)
- **System Status** - Connection and sensor health updates

### 🚫 Alert Cooldown
- **5-minute cooldown** between identical alert types
- **Prevents email spam** during continuous threshold breaches
- **Auto-reset** when conditions return to normal

---

## 📚 Software Dependencies

### Required Arduino Libraries
Install these via Arduino IDE Library Manager:

```cpp
#include <Arduino.h>           // Core Arduino functions
#include <Wire.h>              // I2C communication
#include <Adafruit_GFX.h>      // Graphics library base
#include <Adafruit_SSD1306.h>  // OLED display driver
#include <DHT.h>               // DHT11 sensor library
#include <HX711.h>             // Load cell amplifier
#include <WiFi.h>              // ESP32 WiFi functions
#include <ESP_Mail_Client.h>   // Email sending capability
#include <BlynkSimpleEsp32.h>  // Blynk IoT platform
```

**Installation Command:**
```bash
# Install via Arduino IDE Library Manager or PlatformIO
```

---

## ⚙️ Configuration Instructions

### 1. 📶 WiFi Setup
```cpp
#define WIFI_SSID "Your_WiFi_Network"
#define WIFI_PASSWORD "Your_WiFi_Password"
```

### 2. ☁️ Blynk Configuration
```cpp
#define BLYNK_TEMPLATE_ID "TMPLxxxxxxxxx"
#define BLYNK_TEMPLATE_NAME "Structure Durability"
#define BLYNK_AUTH_TOKEN "Your_32_Character_Auth_Token"
```

**Get your credentials from:** [Blynk Console](https://blynk.cloud/)

### 3. 📧 Email Setup
```cpp
#define AUTHOR_EMAIL "your-monitoring-email@gmail.com"
#define AUTHOR_PASSWORD "your-gmail-app-password"
#define RECIPIENT_EMAIL "alerts-recipient@gmail.com"
```

### 4. 🎛️ Sensor Thresholds
```cpp
int VIBRATION_THRESHOLD = 0;      // Start at 0 for maximum sensitivity
int TILT_ANGLE_THRESHOLD = 2;     // 2 degrees tilt threshold
int TILT_DURATION_THRESHOLD = 1;  // 1 second minimum tilt duration
```

---

## 🧪 Durability Assessment Logic

The system provides intelligent structural health assessment:

| Vibration Count | Status | Expected Lifespan | Recommendation | Alert Level |
|----------------|--------|-------------------|----------------|-------------|
| 0 - 50 | 🟢 Excellent | >30 years | No action needed | ✅ Normal |
| 51 - 500 | 🟡 Good | 20-30 years | Regular monitoring | ⚠️ Watch |
| 501 - 2000 | 🟠 Moderate | 10-20 years | Increased inspection | ⚠️ Caution |
| 2001 - 5000 | 🔴 Poor | 5-10 years | Engineer consultation | 🚨 Warning |
| >5000 | 🚨 Critical | <5 years | Immediate investigation | 🚨 Emergency |

### 📊 Assessment Features
- **Real-time scoring** based on cumulative vibration data
- **Environmental factors** included in assessment
- **Automated reporting** every 5 minutes
- **Trend analysis** for deterioration prediction
- **Professional recommendations** for each risk level

---

## 🚀 Installation Steps

### 1. 🔧 Hardware Assembly
1. Connect all sensors according to the pin configuration table
2. Ensure stable 5V power supply for ESP32
3. Mount sensors securely on the structure to monitor
4. Test all connections with multimeter

### 2. 💻 Software Setup
1. **Clone Repository**

2. **Open in Arduino IDE**

3. **Install Libraries**

4. **Configure Settings**

5. **Upload & Test**

### 3. 📱 Blynk App Configuration
1. Create new project in Blynk app
2. Add widgets for all virtual pins (V0-V14)
3. Configure gauges, LEDs, and displays
4. Set up event notifications
5. Test all controls and displays

### 4. ✅ System Verification
- [ ] All sensors reading correctly
- [ ] OLED display showing data
- [ ] Blynk app receiving updates
- [ ] Email alerts functional
- [ ] Scale calibration complete

---

## 🧩 System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 MAIN CONTROLLER                   │
├─────────────────────────────────────────────────────────────┤
│  Sensors Input          │  Processing         │  Output     │
│  ├─ HX711 (Weight)     │  ├─ Data Collection │ ├─ OLED     │
│  ├─ DHT11 (Temp/Hum)   │  ├─ Threshold Check │ ├─ Blynk    │
│  ├─ SW420 (Vibration)  │  ├─ Alert Logic     │ ├─ Email    │
│  └─ SW18010P (Tilt)    │  └─ Durability Calc │ └─ Serial   │
└─────────────────────────────────────────────────────────────┘
           │                        │                    │
           ▼                        ▼                    ▼
    ┌──────────┐            ┌──────────────┐      ┌─────────────┐
    │ Physical │            │    Blynk     │      │    Email    │
    │Structure │            │Cloud Platform│      │SMTP Server  │
    │Monitoring│            │& Mobile App  │      │   (Gmail)   │
    └──────────┘            └──────────────┘      └─────────────┘
```

--

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE] file for details.

---

## 👨‍🔧 Maintained by

**Vinayak Hegde**
- 🐙 GitHub: [https://github.com/Vinuhegde887]

---

## ⚠️ Safety Disclaimer

**Important:** This system is designed for educational and basic monitoring purposes.


---
