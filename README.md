# CrowPanel Weather + Tides

Weather and tide firmware for the Elecrow CrowPanel ESP32-S3 4.2-inch e-paper HMI display.

This project started from `G6EJD/ESP32-e-Paper-Weather-Display`, but this repository is now focused on one device and one layout:

- Elecrow CrowPanel 4.2-inch e-paper display, 400x300 pixels.
- ESP32-S3 CrowPanel board with the green-driver e-paper revision.
- Norfolk, VA / Sewells Point tide station defaults.
- Open-Meteo weather data, no weather API key required.
- NOAA CO-OPS tide predictions, with local fallback tide samples.
- Pixel-oriented layout tuning for the physical 1-bit e-paper panel.

## Current Layout

The display is a compact weather dashboard:

- Current wind compass with wind speed and direction.
- Current condition icon, temperature, and high/low.
- Current tide height with rising/falling arrow.
- Three near-term forecast boxes for the next 3, 6, and 9 hour forecast slots.
- Weather detail panel with precipitation, visibility, humidity, and cloud cover.
- 24-hour tide graph with sunrise/sunset markers.
- Sunrise, sunset, and moon phase panel.
- 3-day wind, temperature, and rainfall graphs.

## Hardware Notes

The CrowPanel wiring is handled in firmware rather than through GxEPD2:

- Panel power enable: GPIO 7.
- Green-driver enable: GPIO 41.
- SPI SCK: GPIO 12.
- SPI MOSI: GPIO 11.
- E-paper RST: GPIO 47.
- E-paper DC: GPIO 46.
- E-paper CS: GPIO 45.
- E-paper BUSY: GPIO 48.

The display is rendered through a 400x300 `GFXcanvas1` framebuffer and then sent to the panel with the CrowPanel/Elecrow command sequence in `crowpanel_green_epd.h`.

## Data Sources

Weather comes from the Open-Meteo forecast API:

- current temperature, apparent temperature, humidity, cloud cover, visibility, pressure, wind, and weather code
- 72 hours of hourly forecast data
- daily high/low and sunrise/sunset

Tides come from NOAA CO-OPS predictions:

- default station: `8638610`, Sewells Point
- fallback samples live in `examples/CrowPanel_4_2_Weather_Tides/local_tides.h`

Forecast icons are based on Open-Meteo WMO `weather_code`, not directly on rain amount. The rainfall graph uses the hourly `rain` field, so it can show rain even when the three sampled forecast boxes do not show a rain icon.

## Credentials And Location

Do not edit tracked credentials with private data.

Copy the example file:

```sh
cp examples/CrowPanel_4_2_Weather_Tides/owm_credentials_local.example.h \
   examples/CrowPanel_4_2_Weather_Tides/owm_credentials_local.h
```

Then edit `owm_credentials_local.h` with:

- Wi-Fi SSID and password
- latitude/longitude
- city label
- units
- Open-Meteo timezone
- NOAA tide station ID/name

`owm_credentials_local.h` is ignored by git.

## Build And Flash

This repo is set up for PlatformIO. From the CrowPanel example directory:

```sh
cd examples/CrowPanel_4_2_Weather_Tides
../../.venv-platformio/bin/platformio run -e esp32s3
../../.venv-platformio/bin/platformio run -e esp32s3 -t upload --upload-port /dev/cu.usbserial-110
```

Normal runtime behavior:

- wakes every 30 minutes
- fetches weather and tides
- refreshes the screen
- powers down the e-paper hardware
- enters deep sleep

The refresh window is controlled by:

```cpp
long SleepDuration = 30;
int  WakeupTime    = 7;
int  SleepTime     = 23;
```

With the current settings, the device refreshes from 07:00 through 23:59 and skips screen/API refreshes from 00:00 through 06:59.

## Flash Verification

A successful firmware upload does not always mean the panel actually refreshed. The safer workflow is:

1. build and upload `esp32s3`
2. reset/monitor serial
3. confirm the firmware logs `Sending CrowPanel framebuffer`

Example serial success signal:

```text
Sending CrowPanel framebuffer (15000 bytes)
CrowPanel EPD ready after full refresh in 0 ms
Starting deep-sleep period...
```

If the board logs `Failed to obtain time`, reset and let it try again before judging the screen.

## Static Preview Firmware

For no-network layout checks on the physical panel:

```sh
../../.venv-platformio/bin/platformio run -e esp32s3_static -t upload --upload-port /dev/cu.usbserial-110
```

This uses deterministic sample weather and local tide data.

## Software Renderer

The desktop software renderer lives here:

```text
examples/CrowPanel_4_2_Weather_Tides/preview/render_preview.py
```

It renders a 400x300 1-bit PNG from fixture data, useful for quick layout checks before flashing.

Run from the example directory:

```sh
python3 preview/render_preview.py
```

Outputs:

- `preview/software-render-raw-1bit.png`
- `preview/software-render.png`

This is an approximate renderer. It is useful for layout iteration, but the physical panel remains the final check.

## Framebuffer Capture

For pixel-accurate render checks, flash a framebuffer dump build and capture the exact `GFXcanvas1` bytes over serial:

```sh
../../.venv-platformio/bin/platformio run -e esp32s3_dump -t upload --upload-port /dev/cu.usbserial-110
../../.venv-platformio/bin/python preview/capture_framebuffer.py --port /dev/cu.usbserial-110
```

Outputs:

- `preview/framebuffer-capture-raw-1bit.png`
- `preview/framebuffer-capture.png`

There is also a static/no-network dump build:

```sh
../../.venv-platformio/bin/platformio run -e esp32s3_static_dump -t upload --upload-port /dev/cu.usbserial-110
../../.venv-platformio/bin/python preview/capture_framebuffer.py --port /dev/cu.usbserial-110
```

After dump testing, flash the normal `esp32s3` firmware again so the device does not emit framebuffer dumps on every wake.

## Repository Notes

The active firmware is:

```text
examples/CrowPanel_4_2_Weather_Tides/CrowPanel_4_2_Weather_Tides.ino
```

Important support files:

- `examples/CrowPanel_4_2_Weather_Tides/crowpanel_green_epd.h`
- `examples/CrowPanel_4_2_Weather_Tides/local_tides.h`
- `examples/CrowPanel_4_2_Weather_Tides/preview/render_preview.py`
- `examples/CrowPanel_4_2_Weather_Tides/preview/capture_framebuffer.py`
- `src/common.h`

Generated build output, credentials, framebuffer captures, and generated font/gallery captures are ignored.
