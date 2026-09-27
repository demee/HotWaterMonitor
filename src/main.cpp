#include <Arduino.h>
#include "logger.h"
#include "spiffs_storage.h"
#include "temperature_history.h"
#include "temperature_sensors.h"
#include "web_server.h"
#include "wifi_connection.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  logSetup();
  wifiSetup();
  storageSetup();
  webServerSetup();
  temperatureSensorsSetup();
  historySetup();
}

void loop() {
  wifiLoop();
  webServerLoop();
  historyLoop();
  delay(10);
}
