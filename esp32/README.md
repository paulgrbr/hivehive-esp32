# ESP32-CAM Module

## Overview
The ESP32 can capture images through its camera module and create a POST request to a given endpoint.

After flashing the program to the ESP it connects to a WiFi network and starts an infinite loop, capturing an image, store it in its PSRAM, setting up TCP connection to the endpoint and finally the POST request. Debugging output such as errors or HTTP return codes can be seen via Serial output with baud rate of 115200 Hz.

Images will be taken in JPEG format and named: 'esp_capture_YYMMDDhhmmss.jpg'. The resolution can be individually set, maximum possiblie resolution is UXGA (1600 x 1200).

## Usage
**Before compiling and flashing make sure the following configurations are made:**
### WiFi connection
- Before compiling set wifi_config.SSID and wifi_config.PASSWORD to your exact WiFi network credentials in the ESP-CAM.ino file
- Note: The connection must be at 2.4 Ghz. Most routers select the bandwidth automatically but in case the router has disabled its 2.4 channel you must re-enable it for this connections.
- To connect to a mobile hotspot select 'Maximize Compatibility' under the cellphones hotspot settings. This forces the connection to the 2.4 Ghz channel.

### Initialize camera sensor
- Set the image resolution to the desired size by setting the FRAMESIZE argument of the 'initEspCamera()' function in ESP-CAM.ino. You can choose between the following resolutions:
  - FRAMESIZE_QVGA      320 x 240
  - FRAMESIZE_VGA       640x480
  - FRAMESIZE_SVGA      800 x 600
  - FRAMESIZE_SXGA      1280 x 1024
  - FRAMESIZE_UXGA      1600 x 1200
- In function 'configure_camera_sensor() you can set brightness, saturation and vertical flipping of the image'
- In the future, more sensor configuration options will be given

### Upload configuration
- Set the 'UPLOAD_URL' to the URL of your desired endpoint like this: 'https://subhost.host.domain:port/path'
- Giving port, https:// or path are optional. The given URL will be processed by the program for establishing the TCP connection
- Set 'CAPTURE_INTERVAL' to desired pause length between the capture+POST of each image. It is recommended to not set it to 0, to not overload the processor
- CAPTURE_INTERVAL is in miliseconds, so CAPTRUE_INTERVAL = 5000 result in a 5 second pause.

### Compiling and flashing
- Connect ESP32 to your computer and compile and flash via Arduino IDE
- The program is now flashed and will start its capture+upload loop whenever the ESP32 is connected to power
