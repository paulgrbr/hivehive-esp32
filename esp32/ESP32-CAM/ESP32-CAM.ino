#include "esp_camera.h"
#include "esp_init.h"
#include "client.h"
#include <Arduino.h>

/*
  Set endpoint URL here.

  Also a port can be specified if necessary.
*/
char *UPLOAD_URL = "https://hivehive-backend.groeber.cloud/upload";

/* constant for now, maybe we can set this dynamic through server later */
const int CAPTURE_INTERVAL = 5000; // in ms

int counter = 0;

void setup() {

  Serial.println("[ESP] INITIALIZING ESP");

  /*
    ESP PINOUT AND CAMERA INITIALIZATION

    Camera resolution can be one of the following ESP framesize types:
      - FRAMESIZE_QVGA      320 x 240
      - FRAMESIZE_VGA       640x480
      - FRAMESIZE_SVGA      800 x 600
      - FRAMESIZE_SXGA      1280 x 1024
      - FRAMESIZE_UXGA      1600 x 1200
  */
  initEspCamera(FRAMESIZE_SVGA);

  Serial.println("[ESP] CONFIGURING CAMERA SENSOR");
  /*
    CAMERA SENSOR SETUP

    Here we can adjust the image and add effects later, if we want to.
    -> sensor adjustment options are listed here: https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/

    Sensor adjustments could also be made via the server later.
  */

  
  int vflip = 1;
  int brightness = 1;
  int saturation = -1;
  configure_camera_sensor(vflip = vflip,
                          brightness = brightness,
                          saturation = saturation);

  /*
    WIFI SETUP

    WiFi must be 2,4 Ghz as the ESP does not support 5 Ghz.
    -> for mobile hotspot (at least on iOS) go to hotspot settings and enable 'Maximize Compatibility'
  */
  wifi_configuration_t wifi_config;
  wifi_config.SSID = "Vodafone-CAKE";
  wifi_config.PASSWORD = "tYsjat-gakke8-kephaw";

  Serial.printf("[ESP] CONFIGURING WIFI CONNECTION TO %s\n", wifi_config.SSID);
  setupWifiConnection(wifi_config);

  Serial.println("[ESP] SETUP COMPLETE");
  Serial.println("");
  Serial.println("---------------------");
  Serial.println("");
  Serial.println("STARTING CAMERA STREAM");
}


  /*
    Repeated Multipart/Formdata upload to endpoint defined in UPLOAD_URL with interval length CAPTURE_INTERVAL.

    Image filename is of format: 'esp_capture_YYYYMMDDhhmmss.jpg'
  */
void loop() {

  Serial.println("");
  Serial.printf("-- Trying to capture and post image number %d\n", counter++);

  int httpCode = postImage(UPLOAD_URL);
  if (httpCode == -1) {
    Serial.println("---- Camera error. Could not capture image");
    return;
  } else if (httpCode == -2) {
    Serial.println("---- Network error. Could not start the host connection");
    return;
  }  else if (httpCode == -3) {
    Serial.println("---- Data error. Could not send the complete image");
    return;
  } else if (httpCode == -4) {
    Serial.println("---- HTTP error. Invalid or missing HTTP response");
    return;
  }

  Serial.printf("---- %s responded with status: %d\n", UPLOAD_URL, httpCode);

  switch (httpCode) {
    case 200:
    case 201:
        Serial.println("------ Success");
        break;

    case 400:
        Serial.println("------ Bad Request");
        break;

    case 401:
    case 403:
        Serial.println("------ Unauthorized or Forbidden");
        break;

    case 404:
        Serial.println("------ URL Not Found");
        break;

    case 500:
    case 502:
    case 503:
        Serial.println("------ Server-side error");
        break;

    default:
        Serial.printf("------ Unexpected response code: %d\n", httpCode);
        break;
  }

  Serial.printf("---- next image in %d seconds.\n", CAPTURE_INTERVAL / 1000);
  Serial.printf("-- Finished capturing and posting image %d\n", counter);

  //delay(CAPTURE_INTERVAL);
}
