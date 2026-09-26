# WaterMonitor

## Overview
WaterMonitor is an ESP32-C3 firmware + static web UI project for reading two DS18B20 sensors and exposing temperatures over HTTP.
The firmware publishes JSON endpoints, and the browser UI visualizes top and bottom tank readings.

## Hardware + stack
- Board/environment: `esp32-c3-devkitm-1` (`[env:esp32-c3-devkitm-1]` in `platformio.ini`)
- Framework: Arduino (`framework = arduino`)
- Filesystem: SPIFFS (`board_build.filesystem = spiffs`)
- Libraries: `OneWire`, `DallasTemperature`, `ArduinoJson`

## Build and flash
Run from the repository root:

```powershell
$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe run --environment esp32-c3-devkitm-1
```

```powershell
$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe run --environment esp32-c3-devkitm-1 --target uploadfs
```

Files under `data/` are included in the SPIFFS image uploaded by `--target uploadfs`.

## Local Wi-Fi config
Create a local credentials header before building firmware:

```powershell
Copy-Item .\src\local_env.example.h .\src\local_env.h
```

Then edit `src/local_env.h` and set `WIFI_SSID` / `WIFI_PASS`.
`src/local_env.h` is ignored by git, so secrets stay local.

## API
- `GET /` returns `{"status": "ok"}`.
- `GET /api/status` returns static JSON fields `top_c`, `bottom_c`, `heating_pump`, `hot_water_flow`.
- `GET /api/temp` returns JSON generated from DS18B20 readings with keys `tempCold` and `tempHot`.

## Web UI
`data/index.html` polls `/api/temp` every 3 seconds to update top and bottom tank temperature display values.
The page currently reads response fields named `temp1C` (bottom) and `temp2C` (top).

## Project layout
- `src/main.cpp` - firmware setup, Wi-Fi bootstrapping, HTTP handlers, sensor reads.
- `data/index.html` - single-page UI that fetches and renders live temperature values.
- `platformio.ini` - PlatformIO environment, board, filesystem, and library dependencies.
- `partitions.csv` - flash partition map used by the ESP32 build.
