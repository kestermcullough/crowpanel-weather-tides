#ifndef LOCAL_TIDES_H
#define LOCAL_TIDES_H

struct LocalTideSample {
  uint8_t hour;
  float height_ft;
};

static const char LOCAL_TIDE_STATION[] = "Local";
static const char LOCAL_TIDE_DATE[] = "Today";

// Replace these values each day, or generate this file from a tide source.
// Time should be local display time. Height is in feet.
// Keep the final 24-hour point so interpolation works through the end of day.
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

static const int LocalTideSampleCount = sizeof(LocalTideSamples) / sizeof(LocalTideSamples[0]);

#endif
