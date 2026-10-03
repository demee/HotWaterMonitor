#include <Arduino.h>
#include "lcd_display.h"
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
  lcdDisplaySetup();
}

void loop() {
  wifiLoop();
  webServerLoop();
  historyLoop();
  lcdDisplayLoop();
  delay(10);
}
