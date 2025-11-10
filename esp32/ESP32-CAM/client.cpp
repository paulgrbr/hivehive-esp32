#include "esp_camera.h"
#include "client.h"
#include <time.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WifiClientSecure.h>
#include <Arduino.h>

/*
  Extracts
    - host
    - port
    - endpoint path
  from a given URL and sets it to the Url struct.
*/
static url_t splitUrl(const char* urlChars) {
  unsigned long __t_parse_url_start = millis();
  url_t url;
  
  /* default port + path if given URL does not contain any */
  url.port = 443; // https
  url.path = "/";
  
  /*
    Finds the position of the trailing '://' after http/https
    and sets the position of the host address to right after the double slash.

    If no '://' found, the URL most likely starts without 'http(s)://' so hostIndex is 0
  */
  String urlString(urlChars);
  int doubleslashPosition = urlString.indexOf("://");
  int hostIndex = doubleslashPosition >= 0 ? doubleslashPosition + 3 : 0;
  
  /* 
    Finds start of the path (first '/' after hostname).
    Extracts host (+ port) between hostIndex and first slash

    If no '/' the full String after hostIndex is set to the host
  */
  int slash = urlString.indexOf('/', hostIndex);
  String host = slash >= 0 ? urlString.substring(hostIndex, slash) : urlString.substring(hostIndex);
  
  /*
    If slash is found, everything after that is set to the Url path
  */
  if (slash >= 0) {
    url.path = urlString.substring(slash);
  }

  /*
    Separates host address and port and sets the Url struct host and port accordingly.
    
    If no port is found, just the URL host is set.
  */
  int colon = host.indexOf(':');
  if (colon >= 0) {
    url.host = host.substring(0, colon);
    url.port = host.substring(colon + 1).toInt();
  } else {
      url.host = host;
  }

  unsigned long __t_parse_url_end = millis();
  Serial.println(String("---- parse url took ") + String((__t_parse_url_end - __t_parse_url_start) / 1000.0f, 3) + " seconds");

  return url;
}

/*
  Creates unique filename of format: esp_capture_YYYYMMDDhhmmss.jpg
*/
String createFileName() {
  unsigned long __t_create_filename_start = millis();

  struct tm timeinfo;
  bool localTimeAvailable = getLocalTime(&timeinfo, 200); /* up to 200ms timeout for getting the local tikme */

  char buf[64];
  if (localTimeAvailable) {
    snprintf(buf, sizeof(buf),
             "esp_capture_%04d%02d%02d_%02d%02d%02d.jpg",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
  } else {
    /* Fallback if local time not available: add millis (miliseconds since boot) so names stay unique */
    snprintf(buf, sizeof(buf), "esp_capture_unknown_%lu.jpg", (unsigned long)millis());
    Serial.println("WARNING: Unable to get local time while creating image filename.");
  }

  Serial.printf("------ file name: %s\n", buf);
  unsigned long __t_create_filename_end = millis();
  Serial.println(String("---- create filename took ")
    + String((__t_create_filename_end - __t_create_filename_start) / 1000.0f, 3)
    + " seconds");

  return String(buf);
}


int postImage(char *UPLOAD_URL) {
  unsigned long __t_all_start = millis();

  /*
    Image is captured through ESP API
  */
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { return -1; }

  /*
    This little beast creates the HTTP request
  */
  String filename = createFileName();
  const String boundary = "----esp32_boundary";

  const String head =
    String("--") + boundary + "\r\n" +
    "Content-Disposition: form-data; name=\"image\"; filename=\"" + filename + "\"\r\n" +
    "Content-Type: image/jpeg\r\n\r\n";

  const String tail = "\r\n--" + boundary + "--\r\n";

  size_t contentLength = head.length() + fb->len + tail.length();

  /* prepare server connection */
  url_t url = splitUrl(UPLOAD_URL);
  WiFiClientSecure client;
  client.setInsecure();            // TODO: for now insecure w/out verification -> should we add certificate?
  client.setTimeout(15000);

  /*
    Starting TCP connection
  */
  unsigned long __t_conn_start = millis();
  if (!client.connect(url.host.c_str(), url.port)) {
    unsigned long __t_conn_end_fail = millis();
    Serial.println(String("---- TCP connect took ") + String((__t_conn_end_fail - __t_conn_start) / 1000.0f, 3) + " seconds");
    esp_camera_fb_return(fb);
    return -2;
  }
  unsigned long __t_conn_end = millis();
  Serial.println(String("---- TCP connect took ") + String((__t_conn_end - __t_conn_start) / 1000.0f, 3) + " seconds");

  /*
    Another beast that creates the POST request header
  */
  unsigned long __t_hdr_start = millis();
  client.print(String("POST ") + url.path + " HTTP/1.1\r\n");
  client.print(String("Host: ") + url.host + "\r\n");
  client.print("Connection: close\r\n");
  client.print(String("Content-Type: multipart/form-data; boundary=") + boundary + "\r\n");
  client.print(String("Content-Length: ") + contentLength + "\r\n\r\n");
  unsigned long __t_hdr_end = millis();
  Serial.println(String("---- POST headers took ") + String((__t_hdr_end - __t_hdr_start) / 1000.0f, 3) + " seconds");

  /*
    And finally the body with the image (fb) header + data

    Sends chunks of 1024 bytes
  */
  unsigned long __t_upload_start = millis();
  client.print(head);
  size_t sent = 0;
  while (sent < fb->len) {
    size_t chunk = client.write(fb->buf + sent, min((size_t)1024, fb->len - sent));
    if (chunk == 0) {

      // this is only entered if there is an error happening during the connection or a fault with the data
      unsigned long __t_upload_err = millis();
      Serial.println(String("---- upload (partial) took ") + String((__t_upload_err - __t_upload_start) / 1000.0f, 3) + " seconds");
      client.stop();
      esp_camera_fb_return(fb);
      return -3;
    }
    sent += chunk;
  }
  client.print(tail);
  unsigned long __t_upload_end = millis();
  Serial.println(String("---- upload took ") + String((__t_upload_end - __t_upload_start) / 1000.0f, 3) + " seconds");

  /*
    finally the HTTP response
  */
  unsigned long __t_resp_wait_start = millis();
  String status = client.readStringUntil('\n');
  unsigned long __t_resp_wait_end = millis();
  Serial.println(String("---- server response wait took ") + String((__t_resp_wait_end - __t_resp_wait_start) / 1000.0f, 3) + " seconds");

  int code = -4;
  if (status.startsWith("HTTP/1.1 ")) code = status.substring(9, 12).toInt();
  client.stop();

  /*
    ALWAYS free the image from memory otherwise the fun won't last for a long time...
  */
  esp_camera_fb_return(fb);

  unsigned long __t_all_end = millis();
  Serial.println(String("---- total capture+post took ") + String((__t_all_end - __t_all_start) / 1000.0f, 3) + " seconds");

  return code;
}