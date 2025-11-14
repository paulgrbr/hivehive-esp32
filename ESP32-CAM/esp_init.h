#ifndef ESP_INIT_H
#define ESP_INIT_H


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

void initEspCamera(framesize_t framesize);
void configure_camera_sensor(int vflip = 1, int brightness = 1, int saturation = -1);
void setupWifiConnection(wifi_configuration_t wifi_config);

#endif