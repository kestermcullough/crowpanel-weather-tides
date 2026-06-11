# CrowPanel 4.2 Weather + Local Tides

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

## Setup

1. Edit `owm_credentials.h` with Wi-Fi, OpenWeather One Call 3.0 API key, latitude, longitude, city label, units, and timezone.
2. Edit `local_tides.h` with the day's hourly tide samples.
3. Arduino IDE board settings:
   - Board: `ESP32S3 Dev Module`
   - Flash size: `8MB`
   - PSRAM: disabled/not required for the current PlatformIO build
   - Partition scheme: large/Huge APP
   - Upload speed: `115200`
4. Upload over USB-C.

The default refresh is 30 minutes. Keep it at 15 minutes or slower unless you have intentionally raised your OpenWeather One Call usage limit.

## Static Panel Test

Before testing Wi-Fi and OpenWeather credentials, flash the static preview environment:

```sh
../../.venv-platformio/bin/platformio run -e esp32s3_static -t upload
```

This uses deterministic sample weather values and the local tide table, then refreshes the physical CrowPanel with the same firmware layout.

## Tide Data

`local_tides.h` is intentionally simple for the first build. The 24-hour tide graph and current tide status use `LocalTideSamples`:

```cpp
static const LocalTideSample LocalTideSamples[] = {
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

Once the display is working, this can be replaced by a generated file, an SD-card file, or an API fetch from a tide source.

Include a final `{24, ...}` sample so the interpolation works cleanly through the end of the day.

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
