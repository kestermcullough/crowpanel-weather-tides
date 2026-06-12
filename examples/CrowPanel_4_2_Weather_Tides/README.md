# CrowPanel 4.2 Weather + NOAA Tides

This example is adapted from `examples/Waveshare_4_2` for the Elecrow CrowPanel ESP32-S3 4.2-inch E-Paper HMI Display.

## CrowPanel Changes

- Uses CrowPanel display power enable on GPIO 7.
- Also enables GPIO 41 for newer "green circular sticker" CrowPanel revisions.
- Uses CrowPanel e-paper SPI pins: SCK 12, MOSI 11, RST 47, DC 46, CS 45, BUSY 48.
- Uses a 400x300 `GFXcanvas1` framebuffer and Elecrow's newer command sequence instead of GxEPD2.
- Uses a full-screen redraw for a cleaner refresh.
- Replaces the pressure value/trend callout with current tide height plus a rising/falling arrow.
- Replaces the current-condition description band with a 24-hour local tide graph.
- Replaces the bottom-left 3-day pressure graph with a 3-day wind graph.
- Keeps the middle-right sunrise/sunset/moon panel.
- Uses Open-Meteo for no-key weather data.
- Uses NOAA CO-OPS hourly tide predictions, with `local_tides.h` retained as an offline fallback.

## Setup

1. Copy `owm_credentials_local.example.h` to `owm_credentials_local.h`.
2. Edit `owm_credentials_local.h` with Wi-Fi, latitude, longitude, city label, units, timezone, and NOAA tide station.
3. Arduino IDE board settings:
   - Board: `ESP32S3 Dev Module`
   - Flash size: `8MB`
   - PSRAM: disabled/not required for the current PlatformIO build
   - Partition scheme: large/Huge APP
   - Upload speed: `115200`
4. Upload over USB-C.

The default refresh is 30 minutes. Open-Meteo's free API is for non-commercial use and is far above this display's refresh rate. NOAA CO-OPS is also queried once per refresh for hourly tide predictions.

The display refresh window is controlled in the sketch:

```cpp
long SleepDuration = 30;
int  WakeupTime    = 7;
int  SleepTime     = 23;
```

With these defaults, the ESP32 wakes every 30 minutes. It refreshes the screen from 07:00 through 23:59, then skips screen/API refreshes from 00:00 through 06:59. If power is removed and restored, it boots normally and updates again during the refresh window.

## Static Panel Test

Before testing Wi-Fi and live API fetches, flash the static preview environment:

```sh
../../.venv-platformio/bin/platformio run -e esp32s3_static -t upload
```

This uses deterministic sample weather values and the local tide table, then refreshes the physical CrowPanel with the same firmware layout.

## Tide Data

The live sketch fetches hourly NOAA CO-OPS tide predictions for the station in `NOAA_TIDE_STATION`. The default is Sewells Point, VA:

```cpp
#define NOAA_TIDE_STATION "8638610"
#define NOAA_TIDE_STATION_NAME "Sewells Point"
```

`local_tides.h` remains as a fallback if NOAA is unavailable. The 24-hour tide graph and current tide status use `LocalTideSamples`:

```cpp
static LocalTideSample LocalTideSamples[LocalTideSampleMax] = {
  {0, 1.2},
  {3, 0.4},
  {6, 3.8},
  {9, 5.6},
  {12, 2.9},
  {15, 0.7},
  {18, 3.5},
  {21, 5.1},
  {24, 1.4}
};
```

Include a final `{24, ...}` sample so the interpolation works cleanly through the end of the day.

## Weather Data

Weather comes from Open-Meteo's forecast API and does not require an API key. The firmware requests current, hourly, and daily fields, then maps WMO weather codes to the OpenWeather-style icon names already used by the original drawing code.

For Norfolk, the tracked template uses:

```cpp
String LAT              = "36.8508";
String LON              = "-76.2859";
#define OPEN_METEO_TIMEZONE "America%2FNew_York"
```

## Layout Preview

`layout-preview.svg` and `layout-preview.png` are hand-built approximate desktop previews of the 400x300 layout. They are useful for checking big spacing decisions, but final font sizes and e-paper refresh behavior still need to be verified on the CrowPanel.

## Software Renderer

`preview/render_preview.py` is the preferred desktop preview tool. It ports the firmware layout into a 400x300 1-bit Pillow canvas, reads tide samples from `local_tides.h`, and uses `preview/fixture.json` for weather data.

Run it from this example directory:

```bash
python3 preview/render_preview.py
```

Outputs:

- `preview/software-render-raw-1bit.png`: raw 400x300 1-bit preview.
- `preview/software-render.png`: scaled preview for easier inspection.

This is not a hardware framebuffer capture, but it exercises the same layout decisions closely enough to catch text collisions, graph placement, and panel spacing before flashing the CrowPanel.

## Firmware Framebuffer Capture

For pixel-accurate layout checks, build a framebuffer dump environment and capture the exact `GFXcanvas1` bytes over serial:

```bash
../../.venv-platformio/bin/platformio run -e esp32s3_dump -t upload --upload-port /dev/cu.usbserial-110
../../.venv-platformio/bin/python preview/capture_framebuffer.py --port /dev/cu.usbserial-110
```

Outputs:

- `preview/framebuffer-capture-raw-1bit.png`: exact 400x300 framebuffer.
- `preview/framebuffer-capture.png`: scaled preview for inspection.

There is also a deterministic no-network dump environment:

```bash
../../.venv-platformio/bin/platformio run -e esp32s3_static_dump -t upload --upload-port /dev/cu.usbserial-110
../../.venv-platformio/bin/python preview/capture_framebuffer.py --port /dev/cu.usbserial-110
```

After capture testing, flash the normal `esp32s3` environment again so the device does not print framebuffer dumps on every wake.
