# WaterMonitor — agent notes

ESP32-C3 (PlatformIO, Arduino framework) hot-water tank monitor. Two DS18B20 sensors on a OneWire bus on `GPIO4` (index 0 = bottom, index 1 = top). The firmware serves a single-page UI from SPIFFS, logs to flash, and records one top/bottom sample per minute to a history partition (31 days retention). Time comes from NTP (`pool.ntp.org`, `time.google.com`, `time.cloudflare.com`) with timezone `CET-1CEST,M3.5.0,M10.5.0/3` (`LOG_TIMEZONE` in `src/logger.cpp`).


# Project commands

Run commands from the repository root.

## Build

PlatformIO is not available on the terminal `PATH`, so invoke its virtual-environment executable directly:

```powershell
$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe run --environment esp32-c3-devkitm-1
```

- Requires `src/local_env.h` (gitignored): copy `src/local_env.example.h` and set `WIFI_SSID` / `WIFI_PASS`.

## Upload SPIFFS

Files under `data/` are included in the SPIFFS image:

```powershell
$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe run --environment esp32-c3-devkitm-1 --target uploadfs
```

# Architecture

## Modules

- `src/main.cpp` — wiring only. `setup()`: `logSetup`, `wifiSetup`, `storageSetup`, `webServerSetup`, `temperatureSensorsSetup`, `historySetup`, `lcdDisplaySetup`. `loop()`: `wifiLoop`, `webServerLoop`, `historyLoop`, `lcdDisplayLoop`, `delay(10)`.
- `src/logger.*` — `logPrintf()` writes to Serial and `/log.txt` on the `logs` partition (mutex-protected). Rotates to `/log.old.txt` at 100 KB. Timestamps are `+sec.ms` uptime until the clock passes `MIN_VALID_EPOCH` (2024-01-01, i.e. NTP synced). Sets `TZ` at boot.
- `src/wifi_connection.*` — connect, event logging, NTP start (`configTzTime`). Sleep disabled. Checks every 10 s, reconnects every 30 s while down, `ESP.restart()` after 5 min offline (`WIFI_*_MS` constants).
- `src/spiffs_storage.*` — mounts the `spiffs` partition at `/spiffs` (holds `index.html`).
- `src/temperature_sensors.*` — `readTankTemperatures()` → `TankTemperatures{bottom, top}`.
- `src/temperature_history.*` — per-local-day files `/YYYYMMDD.bin` on the `history` partition. Appends one record per wall-clock minute, only after NTP sync. Prunes files older than 31 days when a new day file is created.
- `src/lcd_display.*` — 1602 LCD with PCF8574 I2C backpack, SDA `GPIO5`, SCL `GPIO6`. At boot scans the bus, logs every device, and uses the first address in `0x20-0x27`/`0x38-0x3F`; display disabled if none. Refreshes every 5 s: row 0 top, row 1 bottom; `--.-` for a disconnected sensor. `Wire.begin(SDA, SCL)` must run before `lcd.init()` (the library calls `Wire.begin()` with default pins).
- `src/web_server.*` — HTTP routes (below).
- `src/shower_detector.cpp` — fully commented-out analog draft on `GPIO3`; not compiled.

## Flash partitions (`partitions.csv`)

| Name | Type/Subtype | Size | Use |
|---|---|---|---|
| nvs | data/nvs | 0x6000 | |
| phy_init | data/phy | 0x1000 | |
| app | app/factory | 0x200000 | firmware |
| logs | data/spiffs | 0x60000 | logger, mounted at `/logs` |
| history | data/spiffs | 0x150000 | history, mounted at `/history` |
| spiffs | data/spiffs | 0x40000 | web UI, mounted at `/spiffs` |

PlatformIO's `uploadfs` targets the **last** filesystem partition in the table, so `spiffs` must stay last or the UI image will overwrite logs/history.

## HTTP API

- `GET /` — `index.html` from SPIFFS, else `404 index.html not found`.
- `GET /api/status` — hardcoded placeholder JSON (`top_c`, `bottom_c`, `heating_pump`, `hot_water_flow`); not real data.
- `GET /api/temp` — live readings, e.g. `{"tempBottom":38.1,"tempTop":52.3}`.
- `GET /api/log` — current log as text; `?old` returns the rotated log. `404 log not found` if missing.
- `GET /api/history?day=YYYYMMDD` — raw day file as `application/octet-stream`. `400 day must be YYYYMMDD`, `404 history not found`.

History wire format: packed 8-byte little-endian records `{uint32 epoch, int16 top, int16 bottom}`, temperatures in 1/100 °C, `INT16_MIN` = invalid reading (`HistoryRecord` in `src/temperature_history.cpp`). Parsed by `data/index.html` — change both together.

No CORS headers are sent, so the UI only works when served by the device.

## Web UI (`data/index.html`)

Three tabs: Tank, History, Logs.

- Tank: polls `/api/temp` every 10 s. Color scale `T_MIN=30`..`T_MAX=60` °C; shower-ready icon when top ≥ `T_SHOWER=40`.
- History: ranges 1h/6h/24h/7d/30d, refreshes every 60 s while visible. Fetches day files padded by `DAY_SLACK` (12 h) to cover timezone offsets and caches past days. Marks heating (rise ≥ `HEAT_RISE` 0.8 °C) and shower (drop ≥ `SHOWER_DROP` 1.5 °C) bands over a 15 min trend window.
- Logs: shows `/api/log?old=1` followed by `/api/log`.

## Gotchas

- SPIFFS `open()` returns a valid empty directory handle for missing paths — always check `exists()` first (see `web_server.cpp`, `temperature_history.cpp`, `logger.cpp`).
- Three separate SPIFFS instances: the global `SPIFFS` object is the UI partition; logger and history use their own `fs::SPIFFSFS` objects.
