# CrowPanel Weather/Tide TODO

Working branch: `codex/crowpanel-weather-tides`

## Current State

- Normal device firmware is running with Open-Meteo weather/tide data and the X11 8x13 font pass.
- The framebuffer dump workflow works and is the preferred way to validate layout before flashing/physically checking the e-paper panel.
- The font gallery workflow works; see `FONT.md` for the current font notes and preference order.

## Git/Project Setup

- [ ] Create a fresh standalone GitHub repo for this project instead of continuing long-term in the fork.
- [ ] Decide whether to delete the pushed `codex/crowpanel-weather-tides` branch from the current public fork after the standalone repo exists.
- [x] Keep `owm_credentials_local.h`, framebuffer captures, and generated font gallery PNGs ignored.

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

- [ ] Decide how the 3-day bottom charts should communicate time of day.
  - Problem: rolling 72-hour charts show "how many hours from now", but storms two days out require mental math to know whether they are overnight, morning, midday, or evening.
  - Option A: keep rolling 72 hours from now and add vertical dotted sunrise/sunset markers.
  - Option B: keep rolling 72 hours and add midnight/day-boundary markers plus small day labels.
  - Option C: switch back to fixed calendar-day 24-hour periods.
  - Option D: keep the rolling chart but add a compact "next rain" label/callout with day/time and amount, e.g. `Rain Sat 5a`.
  - Current lean: try Option B plus maybe Option D. It preserves forecast relevance while reducing mental math more than sunrise/sunset alone.
- [ ] Update bottom chart x-axis labels if we add semantic markers.
  - Candidate: replace bare `0 1 2 3` with day labels or mixed labels that still fit.
  - Avoid over-labeling because the charts are small.
- [ ] If using sunrise/sunset markers on bottom charts, make them visually lighter than actual data.
  - Use thin/dotted vertical markers.
  - Consider pairing with midnight separators so sunrise/sunset does not have to carry all day-position meaning.

## Layout/UI Tweaks

- [ ] Overhaul the font/text system so it measures and renders correctly.
  - `drawString()` measures text with Adafruit GFX bounds but renders with U8g2, which can make centering/baselines drift.
  - Current best font for the whole display is X11 8x13; see `FONT.md`.
  - First fix the text measurement/rendering path, then clean up any layout issues it exposes.
  - After that, add larger X11-style fonts selectively for prominent values like current temperature, tide height, and major chart labels.
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
- [ ] Review the middle-right panel spacing after the next live framebuffer capture.
- [ ] Move the wind compass triangle back outward into the ring between the two compass circles.
  - Current position is too far inside.
  - Remove the numeric degree text from the middle; keep only text direction and wind speed.
- [ ] Move the tide arrow down 2 px in the current tide status.
- [ ] Center current temperature and high/low directly under the conditions icon.
  - Move humidity percentage out of that area.
  - Put humidity in the middle-right detail panel.
- [ ] Preemptively shrink the precipitation value in the middle-right detail panel.
  - We have not seen a meaningful precip value on-device yet, so leave extra room now.
- [ ] Change the tide chart top-right value from current tide to max tide.
  - Current tide is already shown in the top-left tide status, so the chart should expose different information.

## Capture/Testing Workflow

- [x] Use `esp32s3_dump` for pixel-accurate live framebuffer captures.
- [x] Use `esp32s3_static_dump` when testing layout without Wi-Fi/API variability.
- [x] After each dump session, flash normal `esp32s3` firmware back to avoid serial dumps on every wake.
- [x] Add font-gallery firmware mode and multi-dump capture support.
- [x] Keep checking the physical e-paper screen after framebuffer changes because ghosting/contrast cannot be captured by the framebuffer PNG.
- [x] Use live framebuffer captures for final layout checks unless we deliberately need static data.
