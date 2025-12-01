#include <WiFi.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

const char *HOST_SSID = "ESP32-Access-Point";
const char *HOST_PASSWORD = "esp-12345";

WiFiServer server(80); // port 80
int server_running = 0;
String sessionToken;

String header;

String cfg_ssid           = "";
String cfg_password       = "";
String cfg_upload_url     = "";
String cfg_resolution     = "VGA";
int    cfg_interval_ms    = 300;
int    cfg_vflip          = 0;
int    cfg_brightness     = 0;
int    cfg_saturation     = 0;


/*
  -----------------------------
  ---------- HELPERS ----------
  -----------------------------
*/
String urlDecode(const String& src) {
  String decoded = "";
  char c;
  for (size_t i = 0; i < src.length(); i++) {
    c = src[i];
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < src.length()) {
      char h1 = src[i + 1];
      char h2 = src[i + 2];
      int hi = isdigit(h1) ? h1 - '0' : toupper(h1) - 'A' + 10;
      int lo = isdigit(h2) ? h2 - '0' : toupper(h2) - 'A' + 10;
      decoded += char(hi * 16 + lo);
      i += 2;
    } else {
      decoded += c;
    }
  }
  return decoded;
}

String getParam(const String& query, const String& name) {
  String key = name + "=";
  int start = query.indexOf(key);
  if (start < 0) return "";
  start += key.length();
  int end = query.indexOf('&', start);
  if (end < 0) end = query.length();
  String value = query.substring(start, end);
  return urlDecode(value);
}

/*
  -------------------------------------
  -- LOAD EXISTING CONFIG TO PREFILL --
  -------------------------------------
*/
void loadConfig() {
  if (!SPIFFS.exists("/config.json")) {
    Serial.println("config.json not found, using defaults");
    return;
  }

  File f = SPIFFS.open("/config.json", "r");
  if (!f) {
    Serial.println("Failed to open config.json");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.print("Failed to parse config.json: ");
    Serial.println(err.f_str());
    return;
  }

  cfg_ssid        = doc["NETWORK"]["SSID"]           | "";
  cfg_password    = doc["NETWORK"]["PASSWORD"]       | "";
  cfg_upload_url  = doc["NETWORK"]["UPLOAD_URL"]     | "";

  cfg_interval_ms = doc["CAMERA"]["CAPTURE_INTERVAL_IN_MS"] | 0;
  cfg_resolution  = doc["CAMERA"]["RESOLUTION"]              | "VGA";
  cfg_vflip       = doc["CAMERA"]["VERTICAL_FLIP"]          | 0;
  cfg_brightness  = doc["CAMERA"]["BRIGHTNESS"]             | 0;
  cfg_saturation  = doc["CAMERA"]["SATURATION"]             | 0;
}

/*
  ----------------------------------
  -- UPDATE JSON CONFIG ON DEVICE --
  ----------------------------------
*/
void saveConfig() {
  StaticJsonDocument<512> doc;

  JsonObject net  = doc.createNestedObject("NETWORK");
  JsonObject cam  = doc.createNestedObject("CAMERA");

  net["SSID"]        = cfg_ssid;
  net["PASSWORD"]    = cfg_password;
  net["UPLOAD_URL"]  = cfg_upload_url;

  cam["CAPTURE_INTERVAL_IN_MS"] = cfg_interval_ms;
  cam["RESOLUTION"]             = cfg_resolution;
  cam["VERTICAL_FLIP"]          = cfg_vflip;
  cam["BRIGHTNESS"]             = cfg_brightness;
  cam["SATURATION"]             = cfg_saturation;

  File f = SPIFFS.open("/config.json", "w");
  if (!f) {
    Serial.println("Failed to open config.json for writing");
    return;
  }
  if (serializeJson(doc, f) == 0) {
    Serial.println("Failed to write JSON to file");
  }
  f.close();
}

/*
  ----------------------------------
  -------- HTML CONFIG FORM --------
  ----------------------------------
*/
void sendConfigForm(WiFiClient &client, bool saved = false) {
  // Optional: split cfg_upload_url into base + endpoint for pre-filling
  String uploadBase = cfg_upload_url;
  String uploadEndpoint = "";
  int lastSlash = uploadBase.lastIndexOf('/');
  if (lastSlash > 7) { // after "http://", "https://"
    uploadEndpoint = uploadBase.substring(lastSlash + 1);
    uploadBase = uploadBase.substring(0, lastSlash);
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<title>ESP32 Config</title>");
  client.println(
    "<style>"
    "body{margin:0;padding:20px;font-family:Helvetica,Arial,sans-serif;background:#f5f5f5;}"
    ".card{max-width:520px;margin:40px auto;background:#fff;border-radius:10px;"
      "box-shadow:0 2px 8px rgba(0,0,0,0.12);padding:24px 26px;box-sizing:border-box;}"
    "h1{margin-top:0;font-size:24px;text-align:center;}"
    "h2{margin-top:24px;font-size:18px;border-bottom:1px solid #eee;padding-bottom:4px;}"
    "label{display:block;margin-top:14px;font-weight:bold;font-size:14px;}"
    "input,select{width:100%;padding:8px 10px;margin-top:6px;border-radius:6px;"
      "border:1px solid #ccc;box-sizing:border-box;font-size:14px;}"
    "input:focus,select:focus{outline:none;border-color:#1976d2;box-shadow:0 0 0 2px rgba(25,118,210,0.18);}"
    "button{margin-top:22px;width:100%;padding:10px 0;font-size:16px;border:none;"
      "border-radius:999px;background:#1976d2;color:#fff;cursor:pointer;font-weight:bold;}"
    "button:hover{background:#1458a3;}"
    ".message{padding:10px 12px;border-radius:6px;background:#e8f5e9;color:#1b5e20;"
      "border:1px solid #c8e6c9;margin-top:10px;margin-bottom:10px;font-size:14px;}"
    ".hint{font-size:12px;color:#777;margin-top:4px;}"
    ".inline{display:flex;gap:8px;}"
    ".inline > div{flex:1;}"
    "</style>"
  );
  client.println("</head><body>");
  client.println("<div class=\"card\">");
  client.println("<h1>ESP32 Configuration</h1>");

  if (saved) {
    client.println("<div class=\"message\"><b>Config saved.</b> You can close this page.</div>");
  }

  // Use POST so SSID/password/URL won't appear in the browser's URL bar
  client.println("<form action=\"/save\" method=\"POST\" autocomplete=\"off\">");
  client.println("<input type=\"hidden\" name=\"session\" value=\"" + sessionToken + "\">");
  
  // --- Network section ---
  client.println("<h2>Network</h2>");

  client.println("<label for=\"ssid\">SSID</label>");
  client.println("<input id=\"ssid\" type=\"text\" name=\"ssid\" value=\"" + cfg_ssid + "\">");

  client.println("<label for=\"password\">Password</label>");
  client.println("<input id=\"password\" type=\"password\" name=\"password\" value=\"" + cfg_password + "\">");
  client.println("<div class=\"hint\">Password will be sent in the request body (not visible in the address bar).</div>");

  client.println("<label for=\"upload_base\">Upload URL</label>");
  client.println("<div class=\"inline\">");
  client.println("<div>");
  client.println("<input id=\"upload_base\" type=\"text\" name=\"upload_base\" "
                 "placeholder=\"http://example.com\" value=\"" + uploadBase + "\">");
  client.println("<div class=\"hint\">Base URL</div>");
  client.println("</div>");
  client.println("<div>");
  client.println("<input id=\"upload_endpoint\" type=\"text\" name=\"upload_endpoint\" "
                 "placeholder=\"upload\" value=\"" + uploadEndpoint + "\">");
  client.println("<div class=\"hint\">Endpoint (path)</div>");
  client.println("</div>");
  client.println("</div>");
  client.println("<div class=\"hint\">Server can combine these as "
                 "<code>http://example.com/upload</code>.</div>");

  // --- Camera section ---
  client.println("<h2>Camera</h2>");

  client.println("<label for=\"interval\">Capture interval (ms)</label>");
  client.println("<input id=\"interval\" type=\"number\" name=\"interval\" min=\"10\" "
                 "value=\"" + String(cfg_interval_ms) + "\">");

  client.println("<label for=\"res\">Resolution</label>");
  client.println("<select id=\"res\" name=\"res\">");
  client.println("<option value=\"qvga\""  + String(cfg_resolution.equalsIgnoreCase("qvga")  ? " selected" : "") + ">QVGA - 320 x 240</option>");
  client.println("<option value=\"vga\""   + String(cfg_resolution.equalsIgnoreCase("vga")   ? " selected" : "") + ">VGA - 640 x 480</option>");
  client.println("<option value=\"qxga\""  + String(cfg_resolution.equalsIgnoreCase("qxga")  ? " selected" : "") + ">QXGA - 800 x 600</option>");
  client.println("<option value=\"sxga\""  + String(cfg_resolution.equalsIgnoreCase("sxga")  ? " selected" : "") + ">SXGA - 1280 x 1024</option>");
  client.println("<option value=\"uxga\""  + String(cfg_resolution.equalsIgnoreCase("uxga")  ? " selected" : "") + ">UXGA - 1600 x 1200</option>");
  client.println("</select>");
  client.println("<div class=\"hint\">Form will submit values like <code>qvga</code>, "
                 "<code>vga</code>, <code>sxga</code>, etc.</div>");

  client.println("<label for=\"vflip\">Vertical flip (0/1)</label>");
  client.println("<input id=\"vflip\" type=\"number\" name=\"vflip\" min=\"0\" max=\"1\" "
                 "value=\"" + String(cfg_vflip) + "\">");

  client.println("<label for=\"bright\">Brightness</label>");
  client.println("<input id=\"bright\" type=\"number\" name=\"bright\" "
                 "value=\"" + String(cfg_brightness) + "\">");

  client.println("<label for=\"sat\">Saturation</label>");
  client.println("<input id=\"sat\" type=\"number\" name=\"sat\" "
                 "value=\"" + String(cfg_saturation) + "\">");

  client.println("<button type=\"submit\">Save configuration</button>");
  client.println("</form>");

  client.println("</div>"); // .card
  client.println("</body></html>");
  client.println();
}


/*
  --------------------------------------
  -------- RUN THE ACCESS POINT --------
  --------------------------------------
*/
void runAccessPoint() {
  server_running = 1;
  
  while (server_running) {
    WiFiClient client = server.available();
    if (client) {
      Serial.println("\n------ CLIENT CONNECTED ------");
      String currentLine = "";
      header = "";

      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          header += c;

          if (c == '\n') {
            // blank line: end of HTTP headers
            if (currentLine.length() == 0) {

              // --------- Parse method + path from first line ---------
              // Example first line: "POST /save HTTP/1.1"
              int methodEnd = header.indexOf(' ');
              int pathStart = methodEnd + 1;
              int pathEnd   = header.indexOf(' ', pathStart);

              String method = header.substring(0, methodEnd);
              String fullPath = header.substring(pathStart, pathEnd);  // "/save" or "/save?ssid=..."

              bool isPost = method.equalsIgnoreCase("POST");
              bool isGet  = method.equalsIgnoreCase("GET");

              // --------- Extract Content-Length (for POST body) ---------
              int contentLength = 0;
              int clIndex = header.indexOf("Content-Length:");
              if (clIndex != -1) {
                int lineEnd = header.indexOf('\n', clIndex);
                String clLine = header.substring(clIndex + 15, lineEnd); // after "Content-Length:"
                clLine.trim();
                contentLength = clLine.toInt();
              }

              // Read body if POST
              String body = "";
              if (isPost && contentLength > 0) {
                while ((int)body.length() < contentLength) {
                  if (client.available()) {
                    char ch = client.read();
                    body += ch;
                  }
                }
              }

              // --------- Handle /save ---------
              if (fullPath.startsWith("/save")) {
                String query;

                if (isPost) {
                  // For POST, form data is in body:
                  // "ssid=...&password=...&upload_base=...&upload_endpoint=...&..."
                  query = body;
                } else if (isGet && fullPath.startsWith("/save?")) {
                  // Backwards-compatible GET handler
                  query = fullPath.substring(String("/save?").length());
                }

                // If we got some parameters, process them
                if (query.length() > 0) {
                  // Only treat as valid if session token matches
                  String sessionParam = getParam(query, "session");
                  if (sessionParam == sessionToken) {
                    cfg_ssid        = getParam(query, "ssid");
                    cfg_password    = getParam(query, "password");

                    // NEW: split URL fields
                    String uploadBase     = getParam(query, "upload_base");
                    String uploadEndpoint = getParam(query, "upload_endpoint");

                    // Normalise and combine into cfg_upload_url
                    uploadBase.trim();
                    uploadEndpoint.trim();

                    if (uploadBase.endsWith("/")) {
                      uploadBase.remove(uploadBase.length() - 1);
                    }
                    if (uploadEndpoint.startsWith("/")) {
                      uploadEndpoint = uploadEndpoint.substring(1);
                    }

                    if (uploadBase.length() > 0 && uploadEndpoint.length() > 0) {
                      cfg_upload_url = uploadBase + "/" + uploadEndpoint;
                    } else {
                      // if no endpoint, just store base
                      cfg_upload_url = uploadBase;
                    }

                    cfg_interval_ms = getParam(query, "interval").toInt();
                    cfg_resolution  = getParam(query, "res");
                    cfg_vflip       = getParam(query, "vflip").toInt();
                    cfg_brightness  = getParam(query, "bright").toInt();
                    cfg_saturation  = getParam(query, "sat").toInt();

                    saveConfig();
                    sendConfigForm(client, true);
                    server_running = 0;
                  } else {
                    // invalid / missing session -> just show form again
                    sendConfigForm(client, false);
                  }
                } else {
                  // /save but no params: show form
                  sendConfigForm(client, false);
                }

              } else {
                // Any other path -> show form
                sendConfigForm(client, false);
              }

              break; // done handling request

            } else {
              currentLine = "";
            }
          } else if (c != '\r') {
            currentLine += c;
          }
        }
      }

      client.stop();
      // Serial.println("Client disconnected");
    }
  }

  Serial.println("Exiting runAccessPoint()");
}

void setupAccessPoint() {
  Serial.println("-- Initializing SPIFFS");
  if (!SPIFFS.begin(true)) {        // true = format if mount fails
    Serial.println("SPIFFS mount failed");
  }

  loadConfig();

  sessionToken = String((uint32_t)esp_random(), HEX);

  Serial.println("-- Setting Access Point");
  WiFi.mode(WIFI_AP_STA);
  bool ok = WiFi.softAP(HOST_SSID, HOST_PASSWORD, 1, 0);
  if (!ok) {
    Serial.println("!!! WiFi.softAP FAILED");
  } else {
    Serial.println("AP started OK");
  }

  WiFi.begin();

  IPAddress IP = WiFi.softAPIP();
  Serial.print("---- AccessPoint IP: ");
  Serial.print(IP);

  server.begin();
  runAccessPoint();
}

