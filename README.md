# IoT-Based Gesture-Controlled Desk Lamp

An intelligent, dual-control desk lamp powered by an **ESP32**, featuring touchless gesture recognition via the **APDS9960** sensor, vibrant addressable illumination via a **WS2812B 16-LED ring**, and a responsive **Wi-Fi web dashboard** for seamless remote customization.

---

## 📺 Demo Video

Check out the lamp in action on YouTube:
[Watch Demo Short](https://youtube.com/shorts/t35KbD851ss?si=EMviSMC5QzbDZBST)

---

## 🌟 Key Features

- **Touchless Gesture Control**: Powered by polled proximity and gesture detection (no interrupt pin required).
  - **Left / Right Swipes**: Cycle smoothly through $5$ distinct white color tones.
  - **Up / Down Swipes**: Dynamically increase or decrease brightness.
  - **Hold & Release**: Hold your hand close for $1\text{--}2$ seconds and pull away to toggle the lamp **ON / OFF**.
- **Wi-Fi Web Dashboard**: Access a built-in web server hosted directly on the ESP32 to control the lamp from any smartphone, tablet, or PC connected to your local network.
  - **14 Color Options**: Choose from $9$ vibrant RGB colors (web-only) and $5$ white color tones (accessible via both web and physical gestures).
  - **Real-Time Sliders**: Fine-tune brightness ($10$ to $255$) and select colors instantly.
- **Wireless OTA Updates**: Support for Over-The-Air firmware updates after the initial USB flash.
- **Portable Power System**: Integrated $18650$ lithium battery setup with charging regulation and step-up boost conversion.

---

## 🛠️ Hardware Components & Bill of Materials (BOM)

| Component | Description |
| :--- | :--- |
| **ESP32-WROOM-32 DevKitC** | Main microcontroller with built-in Wi-Fi and Bluetooth |
| **Adafruit APDS9960 Sensor** | Digital RGB, ambient light, proximity, and gesture sensor ($I^2C$ protocol) |
| **WS2812B 16-LED Ring** | Addressable RGB LED ring for uniform illumination |
| **$18650$ Li-ion Battery ($3.7\,\text{V}$)** | Rechargeable power source |
| **HW-107 Charger Module** | Micro-USB charging module for the $18650$ cell |
| **Slide Switch** | Physical power cutoff switch |
| **Boost Converter Module** | Adjustable DC-DC step-up module (trimmed to $5.0\,\text{V}$) |
| **Resistor** | $330\,\Omega$ series resistor for data line protection |

---

## 🔌 Wiring & Pinout Guide

The system integrates sensors and actuators over $I^2C$ and digital GPIO lines as follows:

### 1. APDS9960 Gesture Sensor ($I^2C$)
- **VCC** $\rightarrow$ $3.3\,\text{V}$
- **GND** $\rightarrow$ $\text{GND}$
- **SDA** $\rightarrow$ $\text{GPIO21}$
- **SCL** $\rightarrow$ $\text{GPIO22}$
*(Note: The INT pin is left unconnected as gesture and proximity data are polled directly in software).*

### 2. WS2812B 16-LED Ring
- **5V / VCC** $\rightarrow$ Boost Converter Output ($5.0\,\text{V}$)
- **GND** $\rightarrow$ $\text{GND}$
- **DIN** $\rightarrow$ $\text{GPIO27}$ (routed through a $330\,\Omega$ resistor)

### 3. Power Architecture
- $18650$ Battery $\rightarrow$ HW-107 Charger $\rightarrow$ Slide Switch $\rightarrow$ Boost Converter $\rightarrow$ ESP32 ($3.3\,\text{V}$ / $\text{VIN}$) & LED Ring ($5.0\,\text{V}$).

---

## 💻 Software & Libraries Required

Ensure you have the following libraries installed via the Arduino IDE Library Manager:
1. **Adafruit APDS9960** (along with its dependencies: `Adafruit BusIO`, `Adafruit Unified Sensor`)
2. **Adafruit NeoPixel**

*Note: `WiFi.h`, `WebServer.h`, `ArduinoOTA.h`, and `Wire.h` are natively included in the ESP32 board core.*

---

## 🚀 Getting Started & Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/your-username/esp32-gesture-desk-lamp.git
   ```
2. **Configure Wi-Fi Credentials**:
   Open the code file and replace the placeholder credentials with your local network details:
   ```cpp
   const char* WIFI_SSID = "Your_SSID";
   const char* WIFI_PASS = "Your_Password";
   ```
3. **Upload the Firmware**:
   - Connect your ESP32 to your PC via USB.
   - Select your board (`ESP32 Dev Module`) and correct COM port in the Arduino IDE.
   - Click **Upload**.
4. **Access the Web Dashboard**:
   - Open the Serial Monitor at `115200` baud.
   - Note the local IP address assigned to the ESP32 (e.g., `http://192.168.1.50/`).
   - Open that URL in any browser on your network to control your lamp remotely!

---

## 📜 License

This project is open-source and available under the [MIT License](LICENSE).