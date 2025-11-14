#include "esp_camera.h"
#include "esp_init.h"
#include "host.h"
#include "client.h"
#include <Arduino.h>
#include <ArduinoJson.h>


#define CONFIG_FILE "/config.json"

esp_config_t esp_config;
int counter = 0;


/*
 * ------------------------------------------------------------------------------
 * PROGRAM START
 * ------------------------------------------------------------------------------
*/
void setup() {
  Serial.setDebugOutput(true);
  Serial.println();
  delay(200);

  /*
    ESP opens WiFi access point to receive the configuration from user input

    Once connected go to:
          ==============================
          ===== http://192.168.4.1 ===== -> ESP softAP() endpoint
          ==============================

    to type in WiFi credentials, endpoint URL and camera settings
  */
  Serial.println("[ESP] OPENING ACCESS POINT");
  setupAccessPoint();


  Serial.println("[ESP] INITIALIZING ESP");

  if (!loadConfig(*esp_config)) {
    Serial.println("-- Failed to configure ESP");
  }

  initEspCamera(RESOLUTION);
  configure_camera_sensor(vflip = vflip,
                          brightness = brightness,
                          saturation = saturation);

  Serial.printf("[ESP] CONFIGURING WIFI CONNECTION TO %s\n", esp_config.wifi_config.SSID);
  setupWifiConnection(*esp_config.wifi_config);

  Serial.println("[ESP] SETUP COMPLETE");
  Serial.println("");
  Serial.println("---------------------");
  Serial.println("");
  Serial.println("STARTING CAMERA STREAM");
}


void loop() {

  Serial.println("");
  Serial.printf("-- Trying to capture and post image number %d\n", counter++);

  int httpCode = postImage(esp_config.UPLOAD_URL);
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

  Serial.printf("-- Finished capturing and posting image %d\n", counter);

  delay(esp_config.CAPTURE_INTERVAL);
}
