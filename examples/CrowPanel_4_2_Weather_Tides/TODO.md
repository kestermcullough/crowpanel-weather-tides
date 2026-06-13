# CrowPanel Weather/Tide TODO

Working branch: `codex/crowpanel-weather-tides`

## Current State

- Normal device firmware is running with Open-Meteo weather/tide data and the ProFont/U8g2 metrics layout pass.
- The framebuffer dump workflow works for exact pixel captures when needed, but normal iteration now uses physical screen checks unless a capture is requested.
- The font gallery workflow works; see `FONT.md` for the current font notes and preference order.

## Git/Project Setup

- [x] Create a fresh standalone GitHub repo for this project instead of continuing long-term in the fork.
- [x] Delete the old public fork after the standalone repo exists.
- [x] Keep `owm_credentials_local.h`, framebuffer captures, and generated font gallery PNGs ignored.
- [x] Ignore duplicate generated `*.ino 2.cpp` artifacts so PlatformIO does not compile the sketch twice.

## Data Validation

- [x] Switch from OpenWeatherMap to Open-Meteo for free hourly/daily coverage.
- [x] Check the thunderstorm/rain mismatch.
  - Open-Meteo did not show the expected evening rain in the live data even though another forecast source did.
  - Decision for now: stick with Open-Meteo and do not add a separate storm cue unless it keeps proving misleading.
- [x] Make the bottom temperature graph a real 3-day graph.
  - Firmware now fetches 72 hours and samples them into 24 plotted points.
- [x] Make the bottom wind graph a real 3-day graph.
  - Same 72-hour-to-24-point sampling as temperature.
- [x] Confirm rainfall graph units and aggregation.
  - Open-Meteo hourly rain is already inches in Imperial mode.
  - Graph now sums 3-hour buckets and keeps decimal Y-axis labels.
- [x] Restore the three top forecast boxes to near-term hourly forecast slots.
  - Boxes now show the next 3/6/9 hour slots, anchored to the previous nearest hour.
- [x] Fix the middle-right weather detail panel.
  - Open-Meteo provides visibility in meters; US display now converts meters to miles correctly.
  - Panel now consistently shows rain/snow amount, visibility, and cloud cover instead of only drawing optional fragments.

## Next Chunk: Forecast Semantics

- [x] Decide how the 3-day bottom charts should communicate time of day.
  - Problem: rolling 72-hour charts show "how many hours from now", but storms two days out require mental math to know whether they are overnight, morning, midday, or evening.
  - Decision: keep rolling 72 hours and add noon/midnight markers.
  - Do not add a "next rain" callout; the graph should be interpretable without doing the interpretation for the user.
- [x] Add noon/midnight markers to the bottom charts.
  - Midnight uses longer dashes; noon uses lighter dotted marks.
- [ ] Review bottom chart x-axis labels after seeing the noon/midnight markers on-device.
  - Keep labels minimal because the charts are small.

## Layout/UI Tweaks

- [x] Overhaul the font/text system so it measures and renders correctly.
  - `drawString()` now measures with U8g2 and renders with U8g2, avoiding the old Adafruit/U8g2 width mismatch.
  - Current best font pass uses ProFont, with larger U8g2 font buckets selected for prominent values.
  - Several values now use measured substring placement for pixel-controlled spacing.
- [x] Remove `RH` text from the upper-left current conditions area.
- [x] Remove decimals from wind and temperature Y-axis labels on the bottom charts.
- [x] Keep decimal Y-axis labels on the rainfall chart.
- [x] Make the three bottom graphs taller.
- [x] Raise and then settle the three bottom graphs.
  - Final adjustment after testing: moved the chart group back down 5 px from the more aggressive raise.
- [x] Fix bottom chart title rendering.
  - Removed the duplicate title draw that made labels look corrupted.
- [x] Move the wind-direction triangle inward so it sits inside the compass ring.
- [x] Keep the current-time marker on the tide chart.
- [x] Update tide chart x-axis labels to `0:00`, `6:00`, `12:00`, `18:00` and drop the final `24`.
- [x] Add thin dashed sunrise/sunset markers to the tide chart.
- [x] Review and tune the middle-right panel spacing after live physical checks.
- [x] Move the wind compass triangle back outward into the ring between the two compass circles.
  - Current position is too far inside.
  - Remove the numeric degree text from the middle; keep only text direction and wind speed.
- [x] Move the tide arrow down 2 px in the current tide status.
- [x] Center current temperature and high/low directly under the conditions icon.
  - Move humidity percentage out of that area.
  - Put humidity in the middle-right detail panel.
- [x] Preemptively shrink the precipitation value in the middle-right detail panel.
  - We have not seen a meaningful precip value on-device yet, so leave extra room now.
- [x] Change the tide chart top-right value from current tide to max tide.
  - Current tide is already shown in the top-left tide status, so the chart should expose different information.

## Capture/Testing Workflow

- [x] Use `esp32s3_dump` for pixel-accurate live framebuffer captures.
- [x] Use `esp32s3_static_dump` when testing layout without Wi-Fi/API variability.
- [x] After each dump session, flash normal `esp32s3` firmware back to avoid serial dumps on every wake.
- [x] Add font-gallery firmware mode and multi-dump capture support.
- [x] Keep checking the physical e-paper screen after framebuffer changes because ghosting/contrast cannot be captured by the framebuffer PNG.
- [x] Use live framebuffer captures for final layout checks unless we deliberately need static data.
