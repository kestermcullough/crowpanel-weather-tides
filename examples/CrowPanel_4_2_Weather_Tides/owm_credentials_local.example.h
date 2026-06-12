// Copy this file to owm_credentials_local.h and fill in local secrets.
// owm_credentials_local.h is ignored by git.

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

String apikey       = "YOUR_OPENWEATHER_API_KEY";
const char server[] = "api.openweathermap.org";

String LAT              = "36.8508";
String LON              = "-76.2859";
String City             = "NORFOLK";
String Country          = "US";
String Language         = "EN";
String Hemisphere       = "north";
String Units            = "I"; // Imperial / US units
const char* Timezone    = "EST5EDT,M3.2.0,M11.1.0";
const char* ntpServer   = "pool.ntp.org";
int   gmtOffset_sec     = -18000;
int   daylightOffset_sec = 3600;
