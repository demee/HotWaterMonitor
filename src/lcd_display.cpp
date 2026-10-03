#include "lcd_display.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>

#include "logger.h"
#include "temperature_sensors.h"

static const int LCD_SDA_PIN = GPIO_NUM_5;
static const int LCD_SCL_PIN = GPIO_NUM_6;
static const uint8_t LCD_COLS = 16;
static const uint8_t LCD_ROWS = 2;
static const uint32_t LCD_REFRESH_MS = 5000;
static const char LCD_DEGREE = (char)223;

static LiquidCrystal_I2C* lcd = nullptr;

// PCF8574 backpacks use 0x20-0x27, PCF8574A backpacks use 0x38-0x3F.
static bool isLcdBackpackAddress(uint8_t addr) {
  return (addr >= 0x20 && addr <= 0x27) || (addr >= 0x38 && addr <= 0x3F);
}

static uint8_t scanForLcd() {
  uint8_t lcdAddr = 0;
  int found = 0;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() != 0) continue;
    found++;
    logPrintf("I2C device found at 0x%02X", addr);
    if (lcdAddr == 0 && isLcdBackpackAddress(addr)) lcdAddr = addr;
  }
  if (found == 0) {
    logPrintf("No I2C devices found (SDA=GPIO%d, SCL=GPIO%d)", LCD_SDA_PIN, LCD_SCL_PIN);
  }
  return lcdAddr;
}

static void printRow(uint8_t row, const char* label, float temp) {
  char buf[LCD_COLS + 1];
  if (temp == DEVICE_DISCONNECTED_C) {
    snprintf(buf, sizeof buf, "%-8s%5s %cC", label, "--.-", LCD_DEGREE);
  } else {
    snprintf(buf, sizeof buf, "%-8s%5.1f %cC", label, temp, LCD_DEGREE);
  }
  // Pad to full width so leftover characters from the previous value are overwritten.
  size_t len = strlen(buf);
  memset(buf + len, ' ', LCD_COLS - len);
  buf[LCD_COLS] = '\0';

  lcd->setCursor(0, row);
  lcd->print(buf);
}

void lcdDisplaySetup() {
  // LiquidCrystal_I2C::init() calls Wire.begin() with default pins; starting the bus first keeps ours.
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);

  uint8_t addr = scanForLcd();
  if (addr == 0) {
    logPrintf("LCD not found, display disabled");
    return;
  }
  logPrintf("LCD using address 0x%02X", addr);

  lcd = new LiquidCrystal_I2C(addr, LCD_COLS, LCD_ROWS);
  lcd->init();
  lcd->backlight();
  lcd->clear();
}

void lcdDisplayLoop() {
  if (!lcd) return;

  static uint32_t lastUpdate = 0;
  static bool first = true;

  uint32_t now = millis();
  if (!first && now - lastUpdate < LCD_REFRESH_MS) return;
  first = false;
  lastUpdate = now;

  TankTemperatures temps = readTankTemperatures();
  printRow(0, "Shower:", temps.top);
  printRow(1, "Feed:", temps.bottom);
}
