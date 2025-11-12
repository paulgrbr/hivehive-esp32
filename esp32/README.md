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
### 2) Wi-Fi Configuration
Set your Wi-Fi credentials in `ESP-CAM.ino`:

```cpp
#define SSID "YourNetworkSSID";
#define wifi_config.PASSWORD "YourNetworkPassword";
```

**Notes**
- The module must connect on 2.4 GHz. If your router disabled 2.4 GHz, re-enable it.
- For a mobile hotspot, enable Maximize Compatibility to force 2.4 GHz.

### Camera initialization
Choose the image resolution by setting the FRAMESIZE argument in initEspCamera().
**Available resolutions:**
  - `FRAMESIZE_QVGA`    320 x 240
  - `FRAMESIZE_VGA`     640x480
  - `FRAMESIZE_SVGA`    800 x 600
  - `FRAMESIZE_SXGA`    1280 x 1024
  - `FRAMESIZE_UXGA`    1600 x 1200

**Example:** Set framesize to VGA:
```cpp
#define RESOLUTION FRAMESIZE_VGA
```

Additional sensor options are made in function `configure_camera_sensor()`.
Default options for brightness, saturation and vertical flipping of the image are given and can be changed. In the future, more sensor configuration options will be added.

### Upload configuration
Specify the 'UPLOAD_URL' to the URL desired endpoint:
```cpp
#define UPLOAD_URL https://subhost.host.domain:port/path
```

The port, path or the http(s):// are optional. The given URL will be processed in whatever format to establish the TCP connection.

Set the capture interval to desired pause length between the capture+POST of each image. It is recommended to not set it to 0, to not overload the processor.
```cpp
#define CAPTURE_INTERVAL 5000 // in miliseconds, so 5 second pause
```

### Compiling and flashing
- Connect ESP32 to your computer and compile and flash via Arduino IDE
- In Arduino IDE select Tools -> Serial Monitor to get debugging output
- The program is now flashed and will start its capture+upload loop whenever the ESP32 is connected to power
