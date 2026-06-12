#ifndef LOCAL_TIDES_H
#define LOCAL_TIDES_H

struct LocalTideSample {
  float hour;
  float height_ft;
};

#ifndef NOAA_TIDE_STATION
#define NOAA_TIDE_STATION "8638610"
#endif

#ifndef NOAA_TIDE_STATION_NAME
#define NOAA_TIDE_STATION_NAME "Sewells Point"
#endif

static const int LocalTideSampleMax = 25;

// Replace these values each day, or generate this file from a tide source.
// Time should be local display time. Height is in feet.
// Keep the final 24-hour point so interpolation works through the end of day.
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

static int LocalTideSampleCount = 9;

#endif
