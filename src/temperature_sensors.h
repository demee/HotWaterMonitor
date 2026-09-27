#pragma once

struct TankTemperatures {
  float bottom;
  float top;
};

void temperatureSensorsSetup();
TankTemperatures readTankTemperatures();
