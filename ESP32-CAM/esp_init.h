#ifndef ESP_INIT_H
#define ESP_INIT_H

#include "esp_camera.h"

typedef struct {
  char SSID[64];
  char PASSWORD[64];
} wifi_configuration_t;

typedef struct {
  char CONFIG_FILE[32];
  wifi_configuration_t wifi_config;
  char UPLOAD_URL[128];
  framesize_t RESOLUTION;
  int CAPTURE_INTERVAL;
  int vertical_flip;
  int brightness;
  int saturation;
} esp_config_t;


bool loadConfig(esp_config_t *esp_config);
void initEspPinout();
void initEspCamera(framesize_t resolution);
void configure_camera_sensor(esp_config_t *esp_config);
void setupWifiConnection(wifi_configuration_t *wifi_config);

#endif