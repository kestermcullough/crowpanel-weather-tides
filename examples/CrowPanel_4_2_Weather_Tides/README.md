# CrowPanel 4.2 Weather + Local Tides

This example is adapted from `examples/Waveshare_4_2` for the Elecrow CrowPanel ESP32-S3 4.2-inch E-Paper HMI Display.

## CrowPanel Changes

- Uses CrowPanel display power enable on GPIO 7.
- Uses CrowPanel e-paper SPI pins: SCK 12, MOSI 11, RST 47, DC 46, CS 45, BUSY 48.
- Uses `GxEPD2_420_GYE042A87` for the 400x300 SSD1683 panel.
- Uses a full-window paged redraw for a cleaner refresh.
- Replaces the lower-right astronomy panel with a local daily tide table and graph.

## Setup

1. Edit `owm_credentials.h` with Wi-Fi, OpenWeather One Call 3.0 API key, latitude, longitude, city label, units, and timezone.
2. Edit `local_tides.h` with the day's local tide events.
3. Arduino IDE board settings:
   - Board: `ESP32S3 Dev Module`
   - Flash size: `8MB`
   - PSRAM: `OPI PSRAM`
   - Partition scheme: large/Huge APP
   - Upload speed: `115200`
4. Upload over USB-C.

The default refresh is 30 minutes. Keep it at 15 minutes or slower unless you have intentionally raised your OpenWeather One Call usage limit.

## Tide Data

`local_tides.h` is intentionally simple for the first build:

```cpp
static const LocalTideEvent LocalTides[] = {
  {"02:14", 0.4, 'L'},
  {"08:28", 5.6, 'H'},
  {"14:41", 0.7, 'L'},
  {"20:52", 5.1, 'H'}
};
```

Once the display is working, this can be replaced by a generated file, an SD-card file, or an API fetch from a tide source.
