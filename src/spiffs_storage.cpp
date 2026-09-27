#include "spiffs_storage.h"

#include <Arduino.h>
#include <SPIFFS.h>

void storageSetup() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount or format SPIFFS");
  }
}
