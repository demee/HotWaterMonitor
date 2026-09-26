# Project commands

Run commands from the repository root.

## Build

PlatformIO is not available on the terminal `PATH`, so invoke its virtual-environment executable directly:

```powershell
$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe run --environment esp32-c3-devkitm-1
```

## Upload SPIFFS

Files under `data/` are included in the SPIFFS image:

```powershell
$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe run --environment esp32-c3-devkitm-1 --target uploadfs
```
