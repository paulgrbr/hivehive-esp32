#include "esp_camera.h"
#include "mbedtls/base64.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <iostream>
#include <HTTPClient.h>

#define CAMERA_MODEL_AI_THINKER

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

// 4 for flash led or 33 for normal led
#define LED_GPIO_NUM   4
#else
#error "Pins not set for camera model"
#endif

// ===========================
// WIFI SETUP (must be 2,4 ghz)
// ===========================
const char *WIFI_SSID = "Vodafone-CAKE";
const char *WIFI_PASSWORD = "tYsjat-gakke8-kephaw";
const char *UPLOAD_URL = "https://hivehive-backend.groeber.cloud/upload";

struct Url { String host; uint16_t port; String path; };
static Url splitUrl(const char* url) {
  Url u; u.port = 443; u.path = "/";
  String s(url); int p = s.indexOf("://"); int i = p >= 0 ? p + 3 : 0;
  int slash = s.indexOf('/', i); String hp = slash >= 0 ? s.substring(i, slash) : s.substring(i);
  if (slash >= 0) u.path = s.substring(slash);
  int col = hp.indexOf(':'); if (col >= 0) { u.host = hp.substring(0, col); u.port = hp.substring(col+1).toInt(); }
  else u.host = hp;
  return u;
}

// --- minimal multipart uploader ---
int postImageMultipart() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return -1;

  const String boundary = "----esp32_boundary";
  const String head =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"image\"; filename=\"photo.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n\r\n";
  const String tail = "\r\n--" + boundary + "--\r\n";
  size_t contentLength = head.length() + fb->len + tail.length();

  Url u = splitUrl(UPLOAD_URL);
  WiFiClientSecure client;
  client.setInsecure();            // for now
  client.setTimeout(15000);

  if (!client.connect(u.host.c_str(), u.port)) { esp_camera_fb_return(fb); return -2; }

  // request line + headers
  client.print(String("POST ") + u.path + " HTTP/1.1\r\n");
  client.print(String("Host: ") + u.host + "\r\n");
  client.print("Connection: close\r\n");
  client.print(String("Content-Type: multipart/form-data; boundary=") + boundary + "\r\n");
  client.print(String("Content-Length: ") + contentLength + "\r\n\r\n");

  // body
  client.print(head);
  size_t sent = 0;
  while (sent < fb->len) {
    size_t n = client.write(fb->buf + sent, min((size_t)1024, fb->len - sent));
    if (n == 0) { client.stop(); esp_camera_fb_return(fb); return -3; }
    sent += n;
  }
  client.print(tail);

  // read status
  String status = client.readStringUntil('\n');
  int code = -4;
  if (status.startsWith("HTTP/1.1 ")) code = status.substring(9, 12).toInt();
  client.stop();

  esp_camera_fb_return(fb);
  return code;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(200);

  // ======== Camera init ========
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
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (psramFound()) {
    Serial.println("PSRAM found");
    config.frame_size   = FRAMESIZE_SVGA;  // 800x600
    config.jpeg_quality = 10;              // 10-20 (lower = better quality)
    config.fb_count     = 2;
    //config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;
  } else {
    Serial.println("PSRAM not found");
    config.frame_size   = FRAMESIZE_VGA;   // 640x480 px
    config.jpeg_quality = 15;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_DRAM;
    //config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[cam] Init failed: 0x%x\n", err);
    while (true) delay(1000);
  } else {
    Serial.println("[cam] initialized");
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); 
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  // ======== Wi-Fi ========
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[wifi] Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[wifi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());

  int code = postImageMultipart();
  Serial.printf("[main] Upload finished with code %d\n", code);
}

void loop() {
  delay(10000);
}
