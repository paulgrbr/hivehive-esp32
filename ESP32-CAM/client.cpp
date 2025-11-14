#include "esp_camera.h"
#include "client.h"
#include <time.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WifiClientSecure.h>
#include <Arduino.h>
#include <ArduinoJson.h>



/*
  Extracts
    - host
    - port
    - endpoint path
  from a given URL and sets it to the Url struct.
*/
static url_t splitUrl(const char* urlChars) {
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
  return url;
}

/*
  Creates unique filename of format: esp_capture_YYYYMMDDhhmmss.jpg
*/
String createFileName() {
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
  return String(buf);
}

/*
  Extracts information about the circle from the HTTP response, detected by the circle detection running on the server
  -> radius
  -> filled or not filled
  -> position
*/
void printResponse(String response) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print("------ JSON parse error: ");
    Serial.println(error.c_str());
  } else {
    Serial.println("----------------------------------------------------------------------");
    Serial.println("------------------------- RESPONSE -----------------------------------");
    Serial.println("------------------------------------------------------------");
    Serial.printf("--------------------- %d circles found ---------------------\n", doc["circles"].size());
    Serial.println("------------------------------------------------------------");

    for (int i = 0; i < doc["circles"].size(); i++) {
      int radius = doc["circles"][i]["radius"];
      const char* status = doc["circles"][i]["status"];
      int x = doc["circles"][i]["x"];
      int y = doc["circles"][i]["y"];

      Serial.printf("--------------------- Circle[%d] radius: %d ---------------------\n", i+1, radius);
      Serial.printf("--------------------- Circle[%d] status: %s ---------------------\n", i+1, status);
      Serial.printf("----------------- Circle[%d] position: (%d, %d)------------------\n", i+1, x, y);
      Serial.println("------------------------------------------------------------");
    }

    const char* message = doc["message"];
    Serial.printf("---- Response message: %s\n ----\n", message);
    Serial.println("----------------------------------------------------------------------");
  }
}

int postImage(char *UPLOAD_URL) {
  unsigned long __t_all_start = millis();

  /*
    Image is captured through ESP API

    flash is activated
  */
  digitalWrite(4, HIGH);
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { return -1; }
  delay(100);
  digitalWrite(4, LOW);

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
  client.setNoDelay(true);         // Disables Nagle (whatever that is but seems to be reducing latency)
  client.setTimeout(8000);

  /*
    Starting TCP connection
  */
  unsigned long __t_conn_start = millis();
  if (!client.connect(url.host.c_str(), url.port)) {
    //unsigned long __t_conn_end_fail = millis();
    //Serial.println(String("---- TCP connect took ") + String((__t_conn_end_fail - __t_conn_start) / 1000.0f, 3) + " seconds");
    esp_camera_fb_return(fb);
    return -2;
  }
  unsigned long __t_conn_end = millis();
  //Serial.println(String("---- TCP connect took ") + String((__t_conn_end - __t_conn_start) / 1000.0f, 3) + " seconds");

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
  //Serial.println(String("---- POST headers took ") + String((__t_hdr_end - __t_hdr_start) / 1000.0f, 3) + " seconds");

  /*
    And finally the body with the image (fb) header + data

    Sends chunks of 4064 bytes (4 kib)
  */
  unsigned long __t_upload_start = millis();
  client.print(head);
  size_t sent = 0;
  while (sent < fb->len) {
    size_t chunk = client.write(fb->buf + sent, min((size_t)4096, fb->len - sent));
    if (chunk == 0) {

      // this is only entered if there is an error happening during the connection or a fault with the data
      unsigned long __t_upload_err = millis();
      //Serial.println(String("---- upload (partial) took ") + String((__t_upload_err - __t_upload_start) / 1000.0f, 3) + " seconds");
      client.stop();
      esp_camera_fb_return(fb);
      return -3;
    }
    sent += chunk;
  }
  client.print(tail);
  unsigned long __t_upload_end = millis();
  //Serial.println(String("---- upload took ") + String((__t_upload_end - __t_upload_start) / 1000.0f, 3) + " seconds");

  /*
    finally the HTTP response
  */
  unsigned long __t_resp_wait_start = millis();
  String status = client.readStringUntil('\n');
  unsigned long __t_resp_wait_end = millis();


  // Skip headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) {
      // Empty line = end of headers
      break;
    }
  }

  // Read the JSON body
  String response = "";
  unsigned long start = millis();
  while (client.connected() || client.available()) {
    if (client.available()) {
      char c = client.read();
      response += c;
    } else if (millis() - start > 5000) { // timeout (optional)
      break;
    }
  }

  /*
  Extract server response JSON to read circle detection output
  Output is

  circles: [

  ],
  message: String
  */

  printResponse(response);
  
  //Serial.println(String("---- server response wait took ") + String((__t_resp_wait_end - __t_resp_wait_start) / 1000.0f, 3) + " seconds");
  //Serial.printf("------ HTTP response: %s\n", status);
  int code = -4;
  if (status.startsWith("HTTP/1.1 ")) code = status.substring(9, 12).toInt();
  client.stop();

  /*
    ALWAYS free the image from memory otherwise the fun won't last for a long time...
  */
  esp_camera_fb_return(fb);

  unsigned long __t_all_end = millis();
  //Serial.println(String("---- total capture+post took ") + String((__t_all_end - __t_all_start) / 1000.0f, 3) + " seconds");

  return code;
}