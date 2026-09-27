#include "spiffs_storage.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include "logger.h"

void storageSetup() {
  if (!SPIFFS.begin(true, "/spiffs", 10, "spiffs")) {
    logPrintf("Failed to mount or format SPIFFS");
  }
}
