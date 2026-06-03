#ifndef LOCAL_TIDES_H
#define LOCAL_TIDES_H

// Replace these values each day, or generate this file from a tide source.
// Time should be local display time. Height is in feet.
struct LocalTideEvent {
  const char* time;
  float height_ft;
  char type; // 'H' or 'L'
};

struct LocalTideSample {
  uint8_t hour;
  float height_ft;
};

static const char LOCAL_TIDE_STATION[] = "Local";
static const char LOCAL_TIDE_DATE[] = "Today";

static const LocalTideEvent LocalTides[] = {
  {"02:14", 0.4, 'L'},
  {"08:28", 5.6, 'H'},
  {"14:41", 0.7, 'L'},
  {"20:52", 5.1, 'H'}
};

static const int LocalTideCount = sizeof(LocalTides) / sizeof(LocalTides[0]);

// Hourly-ish samples for the 24h tide graph and current tide estimate.
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
