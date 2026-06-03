#ifndef LOCAL_TIDES_H
#define LOCAL_TIDES_H

// Replace these values each day, or generate this file from a tide source.
// Time should be local display time. Height is in feet.
struct LocalTideEvent {
  const char* time;
  float height_ft;
  char type; // 'H' or 'L'
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

#endif
