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
int    cfg_vflip          = 1;
int    cfg_brightness     = 1;
int    cfg_saturation     = -1;


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
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<title>ESP32 Config</title>");
  client.println("<style>body{font-family:Helvetica;max-width:500px;margin:40px auto;}label{display:block;margin-top:10px;font-weight:bold;}input,select{width:100%;padding:6px;margin-top:4px;box-sizing:border-box;}button{margin-top:20px;padding:10px 20px;font-size:16px;}</style>");
  client.println("</head><body>");
  client.println("<h1>ESP32 Configuration</h1>");

  if (saved) {
    client.println("<p><b>Config saved. You can close this page.</b></p>");
  }

  client.println("<form action=\"/save\" method=\"GET\">");
  client.println("<input type=\"hidden\" name=\"session\" value=\"" + sessionToken + "\">");
  
  client.println("<h2>Network</h2>");
  client.println("<label>SSID</label>");
  client.println("<input type=\"text\" name=\"ssid\" value=\"" + cfg_ssid + "\">");

  client.println("<label>Password</label>");
  client.println("<input type=\"password\" name=\"password\" value=\"" + cfg_password + "\">");

  client.println("<label>Upload URL</label>");
  client.println("<input type=\"text\" name=\"upload\" value=\"" + cfg_upload_url + "\">");

  client.println("<h2>Camera</h2>");

  client.println("<label>Capture interval (ms)</label>");
  client.println("<input type=\"number\" name=\"interval\" value=\"" + String(cfg_interval_ms) + "\">");

  client.println("<label>Resolution</label>");
  client.println("<input type=\"text\" name=\"res\" value=\"" + cfg_resolution + "\">");

  client.println("<label>Vertical flip (0/1)</label>");
  client.println("<input type=\"number\" name=\"vflip\" value=\"" + String(cfg_vflip) + "\">");

  client.println("<label>Brightness</label>");
  client.println("<input type=\"number\" name=\"bright\" value=\"" + String(cfg_brightness) + "\">");

  client.println("<label>Saturation</label>");
  client.println("<input type=\"number\" name=\"sat\" value=\"" + String(cfg_saturation) + "\">");

  client.println("<button type=\"submit\">Save</button>");
  client.println("</form>");

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

      // read HTTP request
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          header += c;

          if (c == '\n') {
            // end of header (blank line)
            if (currentLine.length() == 0) {
              // decide what to send based on header
              int idx = header.indexOf("GET ");
              int sp  = header.indexOf(' ', idx + 4);
              String firstLine = header.substring(idx + 4, sp); // e.g. "/save?ssid=..."

              if (firstLine.startsWith("/save?")) {
              String query = firstLine.substring(String("/save?").length());

              // only treat as a valid save if the session token matches
              String sessionParam = getParam(query, "session");
              if (sessionParam == sessionToken) {
                cfg_ssid        = getParam(query, "ssid");
                cfg_password    = getParam(query, "password");
                cfg_upload_url  = getParam(query, "upload");

                cfg_interval_ms = getParam(query, "interval").toInt();
                cfg_resolution  = getParam(query, "res");
                cfg_vflip       = getParam(query, "vflip").toInt();
                cfg_brightness  = getParam(query, "bright").toInt();
                cfg_saturation  = getParam(query, "sat").toInt();

                saveConfig();
                sendConfigForm(client, true);

                server_running = 0;
              } else {
                sendConfigForm(client, false);
              }
            } else {
              sendConfigForm(client, false);
            }
              break;
            } else {
              currentLine = "";
            }
          } else if (c != '\r') {
            currentLine += c;
          }
        }
      }
    }

    client.stop();
    //Serial.println("Client not connected");
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

