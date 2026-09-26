#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include "local_env.h"

// Globals 
WebServer server(80);

OneWire oneWire(GPIO_NUM_4);
DallasTemperature sensors(&oneWire);

// WiFi Connection Code 

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       Serial.println("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.printf("Disconnected from WiFi access point, reason: %d\n", info.wifi_sta_disconnected.reason);
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        Serial.println("Lost IP address and IP address is reset to 0"); break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           Serial.println("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            Serial.println("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    Serial.println("Client connected"); break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: Serial.println("Client disconnected"); break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   Serial.println("Assigned IP address to client"); break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:  Serial.println("Received probe request"); break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:         Serial.println("AP IPv6 is preferred"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:        Serial.println("STA IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_GOT_IP6:             Serial.println("Ethernet IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_START:               Serial.println("Ethernet started"); break;
    case ARDUINO_EVENT_ETH_STOP:                Serial.println("Ethernet stopped"); break;
    case ARDUINO_EVENT_ETH_CONNECTED:           Serial.println("Ethernet connected"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:        Serial.println("Ethernet disconnected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:              Serial.println("Obtained IP address"); break;
    default:                                    break;
  }
}

void connectToWiFi() {
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting...");

  Serial.println();
  Serial.println(WiFi.localIP());
}


void scanWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Scanning for Wi-Fi networks...");
  int count = WiFi.scanNetworks();

  if (count == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.printf("Found %d network(s):\n\n", count);

    for (int i = 0; i < count; i++) {
      Serial.printf(
        "%2d: %-32s  RSSI: %4d dBm  Security: %s\n",
        i + 1,
        WiFi.SSID(i).c_str(),
        WiFi.RSSI(i),
        WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured"
      );
    }
  }

  WiFi.scanDelete();
}

// Web Server Code 



void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "index.html not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}


void handleStatus() {
  const char* content = "{\"top_c\":52.3,"
              "\"bottom_c\":38.1,"
              "\"heating_pump\":true,"
              "\"hot_water_flow\":false}";

  server.send(200, "application/json", content);
}

void handleTemperature() {
  sensors.requestTemperatures();

  float tempBottom = sensors.getTempCByIndex(0);
  float tempTop = sensors.getTempCByIndex(1);

  JsonDocument doc;

  doc["tempBottom"] = tempBottom;
  doc["tempTop"] = tempTop;


  String content;
  serializeJson(doc, content);

  server.send(200, "application/json", content);
}

void startServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/temp", HTTP_GET, handleTemperature);
  server.begin();
}

// SPIFFS 

void initSPIFFS(){
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount or format SPIFFS");
  }
}

// Detecting shower 
/* 
bool isShower() {
  unsigned int samples = 0;
unsigned long readValue = 0; 
  int showering = 0;
  long value = 0;
  readValue += analogRead(GPIO_NUM_3);
  
  samples++;
  if (samples > 1000) {
    int avr = readValue/samples;
    samples = 0;
    readValue = 0;
    if (avr > 10) {
      showering = 1;
    } else {
      showering = 0;
    }
    return showering == 1 ? true : false;
  }
  return false;
}
*/
// Main Setup 


void setup() {
  Serial.begin(115200);
  delay(1000);
  scanWiFi();
  connectToWiFi();
  initSPIFFS();
  startServer();
  //analogReadResolution(12);
  sensors.begin();
}

// Main loop

void loop() {
  server.handleClient();
  delay(10);
}
