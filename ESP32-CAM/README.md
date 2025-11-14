# ESP32-CAM Module

## Overview
The **ESP32-CAM** module can capture images through its onboard camera and send them via an HTTP **POST** request to a specified endpoint.

After flashing the program to the ESP32, it connects to a Wi-Fi network and starts an infinite loop where it:
1. Captures an image.
2. Stores it in PSRAM.
3. Establishes a TCP connection to the endpoint.
4. Sends the POST request.

Debug output such as errors or HTTP return codes can be viewed via the **Serial Monitor** at a baud rate of **115200**.

Images are captured in **JPEG** format and named as follows: `esp_capture_YYMMDDhhmmss.jpg`
The resolution is configurable, with a maximum supported resolution of **UXGA (1600 × 1200)**.

## Usage

### 1) Prerequisites
- Arduino IDE installed with ESP32 board support. See https://github.com/espressif/arduino-esp32 for more details.
- ESP32-CAM wired with a USB-to-serial adapter (if needed for flashing)
- 2.4 GHz Wi-Fi network

*Note: All ESP configuration is made in the ESP-CAM.ino file.*

---
### 2) ESP configuration
Edit the config.json to add WiFi credentials, server endpoint URL and addtitional camera settings.

#### Network setup
- WiFi SSID, password and server upload URL must be set in order for the program to work.
- The ESP can only connect on 2.4 GHz bandwidth. If your router disabled 2.4 GHz, re-enable it.
- For a mobile hotspot, enable Maximize Compatibility in hotspot settings to force 2.4 GHz.
- Port, path or the http(s):// of the upload URL are optional. The given URL will be processed in whatever format to establish the TCP connection.

#### Camera settings
Choose the image resolution by setting the RESOLUTION section in config.json to one of the following Strings (casing not important):
**Available resolutions:**
  - `"qvga"`    320 x 240
  - `"vga"`     640x480
  - `"qxga"`    800 x 600
  - `"sxga"`    1280 x 1024
  - `"uxga"`    1600 x 1200

**Note** All camera settings are *optional*. The default settings are:
- RESOLUTION: VGA
- CAPTURE_INTERVAL: 300 ms (0.3 seconds)
- VERTICAL_FLIP: 1 (flipped)
- BRIGHTNESS: 1
- SATURATION: -1

### Compiling and flashing
- Connect ESP32 to your computer and compile and flash via Arduino IDE
- In Arduino IDE select Tools -> Serial Monitor to get debugging output
- The program is now flashed and will start its capture+upload loop whenever the ESP32 is connected to power
