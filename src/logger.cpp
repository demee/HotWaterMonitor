#include "logger.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include <stdarg.h>
#include <time.h>

static const char* LOG_PARTITION_LABEL = "logs";
static const char* LOG_BASE_PATH = "/logs";
static const char* LOG_PATH = "/log.txt";
static const char* LOG_ROTATED_PATH = "/log.old.txt";
static const size_t LOG_MAX_BYTES = 100 * 1024;
static const time_t MIN_VALID_EPOCH = 1704067200; // 2024-01-01, anything earlier means NTP not synced yet

const char* const LOG_TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";

static fs::SPIFFSFS logFS;
static SemaphoreHandle_t logMutex = nullptr;
static bool logFileReady = false;

static void formatTimestamp(char* buffer, size_t size) {
  time_t now = time(nullptr);
  if (now >= MIN_VALID_EPOCH) {
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm);
    return;
  }

  unsigned long ms = millis();
  snprintf(buffer, size, "+%lu.%03lu", ms / 1000, ms % 1000);
}

static void appendToFile(const char* line, size_t len) {
  File file = logFS.open(LOG_PATH, FILE_APPEND);
  if (file && file.size() + len > LOG_MAX_BYTES) {
    file.close();
    logFS.remove(LOG_ROTATED_PATH);
    logFS.rename(LOG_PATH, LOG_ROTATED_PATH);
    file = logFS.open(LOG_PATH, FILE_APPEND);
  }

  if (!file) return;
  file.write((const uint8_t*)line, len);
  file.close();
}

void logSetup() {
  // The RTC keeps time across soft resets, so the zone must be set before the first line, not only at NTP start.
  setenv("TZ", LOG_TIMEZONE, 1);
  tzset();

  logMutex = xSemaphoreCreateMutex();
  logFileReady = logFS.begin(true, LOG_BASE_PATH, 2, LOG_PARTITION_LABEL);

  if (!logFileReady) {
    logPrintf("Failed to mount or format log SPIFFS");
    return;
  }
  logPrintf("Boot");
}

void logPrintf(const char* format, ...) {
  char timestamp[24];
  formatTimestamp(timestamp, sizeof timestamp);

  char message[256];
  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof message, format, args);
  va_end(args);

  char line[288];
  int written = snprintf(line, sizeof line, "%s %s\n", timestamp, message);
  if (written < 0) return;
  size_t len = min((size_t)written, sizeof line - 1);

  if (logMutex == nullptr) {
    Serial.print(line);
    return;
  }

  xSemaphoreTake(logMutex, portMAX_DELAY);
  Serial.print(line);
  if (logFileReady) {
    appendToFile(line, len);
  }
  xSemaphoreGive(logMutex);
}

File logOpen(bool rotated) {
  const char* path = rotated ? LOG_ROTATED_PATH : LOG_PATH;
  if (!logFileReady || !logFS.exists(path)) return File();
  return logFS.open(path, FILE_READ);
}
