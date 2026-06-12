// Change to your WiFi credentials
const char* ssid     = "your_SSID";
const char* password = "your_PASSWORD";

// Weather comes from Open-Meteo and tides come from NOAA CO-OPS. No weather API key is required.
String LAT              = "36.8508";                       // Home location Latitude
String LON              = "-76.2859";                      // Home location Longitude

String City             = "NORFOLK";                       // Display label for your location
String Country          = "US";                            // Your ISO-3166-1 two-letter country code
                                                           // https://en.wikipedia.org/wiki/List_of_ISO_3166_country_codes
String Language         = "EN";                            // Reserved for compatibility with the original examples
                                                           // Examples: Arabic (AR) Czech (CZ) English (EN) Greek (EL) Persian(Farsi) (FA) Galician (GL) Hungarian (HU) Japanese (JA)
                                                           // Korean (KR) Latvian (LA) Lithuanian (LT) Macedonian (MK) Slovak (SK) Slovenian (SL) Vietnamese (VI)
String Hemisphere       = "north";                         // or "south"
String Units            = "I";                             // Use 'M' for Metric or I for Imperial
const char* Timezone    = "EST5EDT,M3.2.0,M11.1.0";        // Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
                                                           // See below for examples
#define OPEN_METEO_TIMEZONE "America%2FNew_York"            // IANA timezone URL-encoded for Open-Meteo
#define NOAA_TIDE_STATION "8638610"                         // NOAA CO-OPS station for Sewells Point, VA
#define NOAA_TIDE_STATION_NAME "Sewells Point"
const char* ntpServer   = "pool.ntp.org";                  // Or, choose a time server close to you, but in most cases it's best to use pool.ntp.org to find an NTP server
                                                           // then the NTP system decides e.g. 0.pool.ntp.org, 1.pool.ntp.org as the NTP syem tries to find  the closest available servers
                                                           // EU "0.europe.pool.ntp.org"
                                                           // US "0.north-america.pool.ntp.org"
                                                           // See: https://www.ntppool.org/en/
int   gmtOffset_sec     = -18000; // US Eastern normal time is UTC-5
int  daylightOffset_sec = 3600; // In the UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset

// Example time zones
//const char* Timezone = "MET-1METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";       // Central Europe
//const char* Timezone = "EST-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "EST5EDT,M3.2.0,M11.1.0";           // EST USA
//const char* Timezone = "CST6CDT,M3.2.0,M11.1.0";           // CST USA
//const char* Timezone = "MST7MDT,M4.1.0,M10.5.0";           // MST USA
//const char* Timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";      // Auckland
//const char* Timezone = "EET-2EEST,M3.5.5/0,M10.5.5/0";     // Asia
//const char* Timezone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3":   // Australia
