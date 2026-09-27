#include <Arduino.h>
#include "spiffs_storage.h"
#include "temperature_sensors.h"
#include "web_server.h"
#include "wifi_connection.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  wifiSetup();
  storageSetup();
  webServerSetup();
  temperatureSensorsSetup();
}

void loop() {
  wifiLoop();
  webServerLoop();
  delay(10);
}
