#include "wifi_connection.h"

#include <Arduino.h>
#include <WiFi.h>
#include "local_env.h"
#include "logger.h"

static const unsigned long WIFI_CHECK_INTERVAL_MS = 10000;
static const unsigned long WIFI_RECONNECT_INTERVAL_MS = 30000;
static const unsigned long WIFI_REBOOT_AFTER_MS = 300000;

static unsigned long lastWiFiCheck = 0;
static unsigned long wifiLostSince = 0;
static unsigned long lastReconnectAttempt = 0;

static void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  logPrintf("[WiFi-event] event: %d", (int)event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               logPrintf("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           logPrintf("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           logPrintf("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            logPrintf("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       logPrintf("Connected to access point"); break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      logPrintf("Disconnected from WiFi access point, reason: %d", (int)info.wifi_sta_disconnected.reason);
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: logPrintf("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      logPrintf("Obtained IP address: %s", WiFi.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        logPrintf("Lost IP address and IP address is reset to 0"); break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          logPrintf("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           logPrintf("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          logPrintf("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              logPrintf("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           logPrintf("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            logPrintf("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    logPrintf("Client connected"); break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: logPrintf("Client disconnected"); break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   logPrintf("Assigned IP address to client"); break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:  logPrintf("Received probe request"); break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:         logPrintf("AP IPv6 is preferred"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:        logPrintf("STA IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_GOT_IP6:             logPrintf("Ethernet IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_START:               logPrintf("Ethernet started"); break;
    case ARDUINO_EVENT_ETH_STOP:                logPrintf("Ethernet stopped"); break;
    case ARDUINO_EVENT_ETH_CONNECTED:           logPrintf("Ethernet connected"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:        logPrintf("Ethernet disconnected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:              logPrintf("Obtained IP address"); break;
    default:                                    break;
  }
}

static void scanWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  logPrintf("Scanning for Wi-Fi networks...");
  int count = WiFi.scanNetworks();

  if (count == 0) {
    logPrintf("No networks found.");
  } else {
    logPrintf("Found %d network(s):", count);

    for (int i = 0; i < count; i++) {
      logPrintf(
        "%2d: %-32s  RSSI: %4d dBm  Security: %s",
        i + 1,
        WiFi.SSID(i).c_str(),
        (int)WiFi.RSSI(i),
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
  configTzTime(LOG_TIMEZONE, "pool.ntp.org", "time.google.com", "time.cloudflare.com");

  logPrintf("Connecting to %s...", WIFI_SSID);
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
      logPrintf("WiFi connection restored");
      wifiLostSince = 0;
    }
    return;
  }

  if (wifiLostSince == 0) {
    wifiLostSince = now;
    lastReconnectAttempt = now;
    logPrintf("WiFi connection lost (status %d)", (int)WiFi.status());
    return;
  }

  if (now - wifiLostSince >= WIFI_REBOOT_AFTER_MS) {
    logPrintf("WiFi down for 5 minutes, restarting");
    delay(100);
    ESP.restart();
  }

  if (now - lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
    lastReconnectAttempt = now;
    logPrintf("Reconnecting to WiFi (status %d)...", (int)WiFi.status());
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}
