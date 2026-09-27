#include "wifi_connection.h"

#include <Arduino.h>
#include <WiFi.h>
#include "local_env.h"

static const unsigned long WIFI_CHECK_INTERVAL_MS = 10000;
static const unsigned long WIFI_RECONNECT_INTERVAL_MS = 30000;
static const unsigned long WIFI_REBOOT_AFTER_MS = 300000;

static unsigned long lastWiFiCheck = 0;
static unsigned long wifiLostSince = 0;
static unsigned long lastReconnectAttempt = 0;

static void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
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

static void scanWiFi() {
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

static void connectToWiFi() {
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting...");

  Serial.println();
  Serial.println(WiFi.localIP());
}

void wifiSetup() {
  scanWiFi();
  connectToWiFi();
}

void wifiLoop() {
  unsigned long now = millis();
  if (now - lastWiFiCheck < WIFI_CHECK_INTERVAL_MS) return;
  lastWiFiCheck = now;

  if (WiFi.status() == WL_CONNECTED) {
    if (wifiLostSince != 0) {
      Serial.println("WiFi connection restored");
      wifiLostSince = 0;
    }
    return;
  }

  if (wifiLostSince == 0) {
    wifiLostSince = now;
    lastReconnectAttempt = now;
    Serial.printf("WiFi connection lost (status %d)\n", WiFi.status());
    return;
  }

  if (now - wifiLostSince >= WIFI_REBOOT_AFTER_MS) {
    Serial.println("WiFi down for 5 minutes, restarting");
    delay(100);
    ESP.restart();
  }

  if (now - lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
    lastReconnectAttempt = now;
    Serial.printf("Reconnecting to WiFi (status %d)...\n", WiFi.status());
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}
