#include "esp_camera.h"
#include "esp_init.h"
#include "client.h"
#include <Arduino.h>
#include <ArduinoJson.h>


#define CONFIG_FILE "/config.json"

esp_config_t esp_config;
int counter = 0;

framesize_t getResolutionFromString(const String &resolutionString) {
  switch (resolutionString.toLowerCase()) {
    catch "qvga":
      return FRAMESIZE_QVGA;
      break;
    catch "vga":
      return FRAMESIZE_VGA;
      break;
    catch "svga":
      return FRAMESIZE_SVGA;
      break;
    catch "sxga":
      return FRAMESIZE_SXGA;
      break;
    catch "uxga":
      return FRAMESIZE_UXGA;
      break;
    default:
      Serial.printf("------ Resolution '%s' is not supported. Using Default resolution VGA.\n", resolutionString);
      return FRAMESIZE_VGA;
  }
}

bool loadConfig() {
  /* DEFAULTS */
  esp_config.RESOLUTION = FRAMESIZE_VGA;
  esp_config.CAPTURE_INTERVAL = 300;
  esp_config.vertical_flip = 1;
  esp_config.brightness = 1;
  esp_config.saturation = -1;

  if (!SPIFFS.begin(true)) {
    Serial.println("-- SPIFFS mount failed");
    return false;
  }

  File file = SPIFFS.open("CONFIG_FILE", "r");
  if (!file) {
    Serial.printf("%s not found\n", CONFIG_FILE);
    return false;
  }

  StaticJsonDocument<512> esp_config_doc;
  DeserializationError err = deserializeJson(esp_config_doc, file);
  file.close();
  if (err) {
    Serial.println("JSON parse error");
    return false;
  }

  esp_config.wifi_config.SSID = esp_config_doc["NETWORK"]["SSID"];
  esp_config.wifi_config.PASSWORD = esp_config_doc["NETWORK"]["PASSWORD"];
  esp_config.UPLOAD_URL = esp_config_doc["NETWORK"]["UPLOAD_URL"];
  esp_config.CAPTURE_INTERVAL = esp_config_doc["CAMERA"]["CAPTURE_INTERVAL_IN_MS"];
  esp_config.RESOLUTION = getResolutionFromString(esp_config_doc["CAMERA"]["RESOLUTION"]);
  esp_config.vertical_flip = esp_config_doc["CAMERA"]["VERTICAL_FLIP"];
  esp_config.brightness = esp_config_doc["CAMERA"]["BRIGHTNESS"];
  esp_config.saturation = esp_config_doc["CAMERA"]["SATURATION"];
  
  if (!esp_config.wifi_config.SSID) {
    Serial.println("------ Could not read SSID from config file.");
    return false;
  } else if (!esp_config.wifi_config.PASSWORD) {
    Serial.println("------ Could not read PASSWORD from config file.");
    return false;
  } else if (!esp_config.UPLOAD_URL) {
    Serial.println("------ Could not read UPLOAD_URL from config file.");
    return false;
  }
  return true;
}


/*
 * ------------------------------------------------------------------------------
 * PROGRAM START
 * ------------------------------------------------------------------------------
*/
void setup() {

  Serial.println("[ESP] INITIALIZING ESP");

  if (!loadConfig()) {
    Serial.println("-- Failed to configure ESP");
  }
  initEspCamera(RESOLUTION);
  configure_camera_sensor(vflip = vflip,
                          brightness = brightness,
                          saturation = saturation);

  Serial.printf("[ESP] CONFIGURING WIFI CONNECTION TO %s\n", wifi_config.SSID);
  setupWifiConnection(wifi_config);

  Serial.println("[ESP] SETUP COMPLETE");
  Serial.println("");
  Serial.println("---------------------");
  Serial.println("");
  Serial.println("STARTING CAMERA STREAM");
}


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

  Serial.printf("-- Finished capturing and posting image %d\n", counter);

  delay(CAPTURE_INTERVAL);
}
