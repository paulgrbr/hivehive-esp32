#include "esp_camera.h"
#include "esp_wifi.h"
#include "esp_init.h"
#include <Arduino.h>
#include <WiFi.h>

/* 
  pinouts defined based on the Arduino ESP CameraWebServer example
*/
#define CAMERA_MODEL_AI_THINKER // our ESP model

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

/* 4 for flash led or 33 for normal led */
#define LED_GPIO_NUM   4
#else
#error "Pins not set for camera model"
#endif

/*
  POSIX timezone + NTP servers

  These are needed to set ESP local time after connecting to WiFi
*/
static const char* TZ_EU_CENTRAL = "CET-1CEST,M3.5.0,M10.5.0/3";
static const char* NTP1 = "pool.ntp.org";
static const char* NTP2 = "time.google.com";

/*
  we can modify image with this sensor

  e.g. set brightness, saturation etc.
*/
sensor_t *s;
int initialized = 0;

void configure_camera_sensor(int vflip, int brightness, int saturation) {
  if (initialized) {
    s = esp_camera_sensor_get();

    s->set_vflip(s, vflip);                     // Flips the image vertically (some cameras mount upside-down)
    s->set_brightness(s, brightness);           // Slightly increases brightness
    s->set_saturation(s, saturation);           // Reduces color saturation (for less "washed-out" images)

    /* --- we can add more here --- */
    /* https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/ */
  } else {
    Serial.println("---- Error when configuring camera sensor: Camera not initialized yet");
  }
}

void initEspCamera(framesize_t framesize) {

  Serial.println("-- setting serial output");
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(200);

  Serial.println("-- configuring ESP pinout");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  pinMode(LED_GPIO_NUM, OUTPUT);

  Serial.println("-- configuring camera");

  config.frame_size = framesize;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (psramFound()) {
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // this is just a fallback... don't know if the psram will ever not be found
    Serial.println("---- PSRAM not found. Image quality will be reduced.");
    config.frame_size   = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_DRAM;
  }

  Serial.println("-- initializing ESP camera");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("---- camera init failed: 0x%x\n", err);
    while (true) delay(1000);
  } else {
    initialized = 1;
    Serial.println("---- camera initialized");
  }
}

/*
    Sets local time based on CEST from time servers (pool.ntp.org and time.google.com)
*/
void setupTime() {
  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(TZ_EU_CENTRAL, NTP1, NTP2);

    struct tm tmcheck;
    const uint32_t start = millis();
    while (!getLocalTime(&tmcheck, 500 /*ms timeout*/)) {
      if (millis() - start > 5000) {
        Serial.println("------ WARNING: NTP time sync timed out.");
        Serial.println("------ Could not sync time from NTP servers. Local time will be unavailable.");
        break;
      }
    }
  } else {
    Serial.println("------ WiFi not available, could not sync time from NTP servers. Local time will be unavailable.");
  }
}

void tuneWifiForLatency() {
  WiFi.setSleep(false);                    // Disable modem sleep (lowers jitter)
  esp_wifi_set_ps(WIFI_PS_NONE);           // Same idea at IDF level
  WiFi.setTxPower(WIFI_POWER_19_5dBm);     // Max TX power (if allowed)
}

void setupWifiConnection(wifi_configuration_t wifi_config) {
  char *ssid = wifi_config.SSID;
  char *pw = wifi_config.PASSWORD;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  Serial.printf("---- connecting to %s", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n---- Connected. IP: %s\n", WiFi.localIP().toString().c_str());

  Serial.println("---- Setting up local time from NTP servers");
  setupTime();

  tuneWifiForLatency();
}
