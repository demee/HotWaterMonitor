#include "web_server.h"

#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "logger.h"
#include "temperature_history.h"
#include "temperature_sensors.h"

static WebServer server(80);

static void handleRoot() {
  // SPIFFS.open() returns a valid empty directory handle for missing paths, so check exists() first.
  if (!SPIFFS.exists("/index.html")) {
    server.send(404, "text/plain", "index.html not found");
    return;
  }
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "index.html not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

static void handleStatus() {
  const char* content = "{\"top_c\":52.3,"
              "\"bottom_c\":38.1,"
              "\"heating_pump\":true,"
              "\"hot_water_flow\":false}";

  server.send(200, "application/json", content);
}

static void handleTemperature() {
  TankTemperatures temps = readTankTemperatures();

  JsonDocument doc;

  doc["tempBottom"] = temps.bottom;
  doc["tempTop"] = temps.top;

  String content;
  serializeJson(doc, content);

  server.send(200, "application/json", content);
}

static void handleLog() {
  File file = logOpen(server.hasArg("old"));
  if (!file) {
    server.send(404, "text/plain", "log not found");
    return;
  }
  server.streamFile(file, "text/plain");
  file.close();
}

static void handleHistory() {
  String day = server.arg("day");
  bool valid = day.length() == 8;
  for (size_t i = 0; valid && i < day.length(); i++) {
    valid = isDigit(day[i]);
  }
  if (!valid) {
    server.send(400, "text/plain", "day must be YYYYMMDD");
    return;
  }

  File file = historyOpenDay(day.c_str());
  if (!file) {
    server.send(404, "text/plain", "history not found");
    return;
  }
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void webServerSetup() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/temp", HTTP_GET, handleTemperature);
  server.on("/api/log", HTTP_GET, handleLog);
  server.on("/api/history", HTTP_GET, handleHistory);
  server.begin();
}

void webServerLoop() {
  server.handleClient();
}
