// Copy this file to owm_credentials_local.h and fill in local secrets/settings.
// owm_credentials_local.h is ignored by git.

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

String LAT              = "36.8508";
String LON              = "-76.2859";
String City             = "NORFOLK";
String Country          = "US";
String Language         = "EN";
String Hemisphere       = "north";
String Units            = "I"; // Imperial / US units
const char* Timezone    = "EST5EDT,M3.2.0,M11.1.0";
#define OPEN_METEO_TIMEZONE "America%2FNew_York"
#define NOAA_TIDE_STATION "8638610"
#define NOAA_TIDE_STATION_NAME "Sewells Point"
const char* ntpServer   = "pool.ntp.org";
int   gmtOffset_sec     = -18000;
int   daylightOffset_sec = 3600;
