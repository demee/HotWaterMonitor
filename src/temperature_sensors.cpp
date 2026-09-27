#include "temperature_sensors.h"

#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire oneWire(GPIO_NUM_4);
static DallasTemperature sensors(&oneWire);

void temperatureSensorsSetup() {
  sensors.begin();
}

TankTemperatures readTankTemperatures() {
  sensors.requestTemperatures();
  return {sensors.getTempCByIndex(0), sensors.getTempCByIndex(1)};
}
