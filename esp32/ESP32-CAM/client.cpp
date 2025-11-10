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
  if (!getLocalTime(&timeinfo)) {
    return "esp_capture_unknown.jpg";
  }

  char buf[32];
  snprintf(buf, sizeof(buf),
           "esp_capture_%04d%02d%02d%02d%02d%02d.jpg",
           timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1,
           timeinfo.tm_mday,
           timeinfo.tm_hour,
           timeinfo.tm_min,
           timeinfo.tm_sec);

  return String(buf);
}


int postImage(char *UPLOAD_URL) {
  /*
    Image is captured through ESP API
  */
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { return -1; }

  /*
    This little beast creates the HTTP request
  */
  String filename = createFileName();   // <-- missing semicolon fixed
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
  if (!client.connect(url.host.c_str(), url.port)) {
    esp_camera_fb_return(fb);
    return -2;
  }

  /*
    Another beast that creates the POST request header
  */
  client.print(String("POST ") + url.path + " HTTP/1.1\r\n");
  client.print(String("Host: ") + url.host + "\r\n");
  client.print("Connection: close\r\n");
  client.print(String("Content-Type: multipart/form-data; boundary=") + boundary + "\r\n");
  client.print(String("Content-Length: ") + contentLength + "\r\n\r\n");

  /*
    And finally the body with the image (fb) header + data

    Sends chunks of 1024 bytes
  */
  client.print(head);
  size_t sent = 0;
  while (sent < fb->len) {
    size_t chunk = client.write(fb->buf + sent, min((size_t)1024, fb->len - sent));
    if (chunk == 0) {

      // this is only entered if there is an error happening during the connection or a fault with the data
      client.stop();
      esp_camera_fb_return(fb);
      return -3;
    }
    sent += chunk;
  }
  client.print(tail);

  /*
    finally the HTTP response
  */
  String status = client.readStringUntil('\n');
  int code = -4;
  if (status.startsWith("HTTP/1.1 ")) code = status.substring(9, 12).toInt();
  client.stop();

  /*
    ALWAYS free the image from memory otherwise the fun won't last for a long time...
  */
  esp_camera_fb_return(fb);
  return code;
}