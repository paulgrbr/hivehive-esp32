#ifndef ESP_INIT_H
#define ESP_INIT_H

#include "esp_camera.h"

typedef struct {
  char *SSID;
  char *PASSWORD;
} wifi_configuration_t;

typedef struct {
  wifi_configuration_t wifi_config;
  char *UPLOAD_URL;
  framesize_t RESOLUTION;
  int CAPTURE_INTERVAL;
  int vertical_flip;
  int brightness;
  int saturation;
} esp_config_t;

framesize_t getResolutionFromString(String &resolutionString);
bool loadConfig(esp_config_t *esp_config);
void initEspCamera(framesize_t framesize);
void configure_camera_sensor();
void setupWifiConnection(wifi_configuration_t *wifi_config);

#endif