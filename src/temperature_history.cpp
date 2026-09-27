#include "temperature_history.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include <math.h>
#include <vector>
#include "logger.h"
#include "temperature_sensors.h"

static const char* HISTORY_PARTITION_LABEL = "history";
static const char* HISTORY_BASE_PATH = "/history";
static const int HISTORY_RETENTION_DAYS = 31;

// Wire format of /api/history, read by index.html: 8 bytes little-endian, temperatures in 1/100 °C.
struct __attribute__((packed)) HistoryRecord {
  uint32_t epoch;
  int16_t top;
  int16_t bottom;
};

static fs::SPIFFSFS historyFS;
static bool historyReady = false;

static int16_t encodeTemperature(float celsius) {
  if (isnan(celsius) || celsius < -100.0f) return INT16_MIN;
  return (int16_t)lroundf(celsius * 100.0f);
}

static void formatDay(time_t when, char* buffer, size_t size) {
  struct tm tm;
  localtime_r(&when, &tm);
  strftime(buffer, size, "%Y%m%d", &tm);
}

static void pruneOldDays(time_t now) {
  char cutoff[9];
  formatDay(now - HISTORY_RETENTION_DAYS * 86400, cutoff, sizeof cutoff);

  std::vector<String> expired;
  File root = historyFS.open("/");
  for (String name = root.getNextFileName(); name.length() > 0; name = root.getNextFileName()) {
    String base = name.substring(name.lastIndexOf('/') + 1);
    if (base.length() == 12 && base.endsWith(".bin") && strncmp(base.c_str(), cutoff, 8) < 0) {
      expired.push_back(base);
    }
  }
  root.close();

  for (const String& base : expired) {
    historyFS.remove("/" + base);
  }
}

void historySetup() {
  historyReady = historyFS.begin(true, HISTORY_BASE_PATH, 5, HISTORY_PARTITION_LABEL);
  if (!historyReady) {
    logPrintf("Failed to mount or format history SPIFFS");
  }
}

void historyLoop() {
  if (!historyReady) return;

  time_t now = time(nullptr);
  if (now < MIN_VALID_EPOCH) return;

  static uint32_t lastMinute = 0;
  uint32_t minute = now / 60;
  if (minute == lastMinute) return;
  lastMinute = minute;

  TankTemperatures temps = readTankTemperatures();
  HistoryRecord record = {(uint32_t)now, encodeTemperature(temps.top), encodeTemperature(temps.bottom)};

  char day[9];
  formatDay(now, day, sizeof day);
  String path = String("/") + day + ".bin";
  if (!historyFS.exists(path)) {
    pruneOldDays(now);
  }

  File file = historyFS.open(path, FILE_APPEND);
  bool ok = file && file.write((const uint8_t*)&record, sizeof record) == sizeof record;
  if (file) file.close();

  static bool writeFailing = false;
  if (!ok && !writeFailing) logPrintf("History write failed");
  if (ok && writeFailing) logPrintf("History write recovered");
  writeFailing = !ok;
}

File historyOpenDay(const char* day) {
  String path = String("/") + day + ".bin";
  // SPIFFS open() returns a valid empty directory handle for missing paths, so check exists() first.
  if (!historyReady || !historyFS.exists(path)) return File();
  return historyFS.open(path, FILE_READ);
}
