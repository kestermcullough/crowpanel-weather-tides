# CrowPanel 4.2 Weather + Local Tides

This example is adapted from `examples/Waveshare_4_2` for the Elecrow CrowPanel ESP32-S3 4.2-inch E-Paper HMI Display.

## CrowPanel Changes

- Uses CrowPanel display power enable on GPIO 7.
- Uses CrowPanel e-paper SPI pins: SCK 12, MOSI 11, RST 47, DC 46, CS 45, BUSY 48.
- Uses `GxEPD2_420_GYE042A87` for the 400x300 SSD1683 panel.
- Uses a full-window paged redraw for a cleaner refresh.
- Replaces the pressure value/trend callout with current tide height plus a rising/falling arrow.
- Replaces the current-condition description band with a 24-hour local tide graph.
- Replaces the bottom-left 3-day pressure graph with a 3-day wind graph.
- Replaces the lower-right astronomy panel with a local daily tide table and small tide graph.

## Setup

1. Edit `owm_credentials.h` with Wi-Fi, OpenWeather One Call 3.0 API key, latitude, longitude, city label, units, and timezone.
2. Edit `local_tides.h` with the day's local tide events and hourly tide samples.
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

The 24-hour tide graph uses `LocalTideSamples`. Include a final `{24, ...}` sample so the interpolation works cleanly through the end of the day.

## Layout Preview

`layout-preview.svg` and `layout-preview.png` are approximate desktop previews of the 400x300 layout. They are useful for checking big spacing decisions, but final font sizes and e-paper refresh behavior still need to be verified on the CrowPanel.
