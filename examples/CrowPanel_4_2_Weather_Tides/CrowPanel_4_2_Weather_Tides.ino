/* ESP32 Weather Display using an EPD 4.2" Display, obtains data from Open Weather Map, decodes it and then displays it.
  ####################################################################################################################################
  This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.

  Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
  1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
  2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
  3. You may not, except with my express written permission, distribute or commercially exploit the content.
  4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.
  5. You may not use this software to create YouTube or other video content, or for any purposes of monetisation.

  The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
  software use is visible to an end-user.

  THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY
  OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  See more at http://www.dsbird.org.uk
*/
#if __has_include("owm_credentials_local.h")
#include "owm_credentials_local.h"  // Local, ignored credentials override
#else
#include "owm_credentials.h"        // Tracked placeholder credentials
#endif
#define CROWPANEL_USE_OPEN_METEO 1
#include "local_tides.h"      // Runtime NOAA tide table with local fallback samples
#include <ArduinoJson.h>       // https://github.com/bblanchon/ArduinoJson
#include <WiFi.h>              // Built-in
#include <WiFiClientSecure.h>
#include "time.h"              // Built-in
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "crowpanel_green_epd.h"
#include "epaper_fonts.h"
#include "lang.h"
//#include "lang_cz.h"                // Localisation (Czech)
//#include "lang_fr.h"                // Localisation (French)
//#include "lang_gr.h"                // Localisation (German)
//#include "lang_it.h"                // Localisation (Italian)
//#include "lang_nl.h"                // Localisation (Dutch)
//#include "lang_pl.h"                // Localisation (Polish)

#define SCREEN_WIDTH  400.0    // Set for landscape mode, don't remove the decimal place!
#define SCREEN_HEIGHT 300.0

#define GxEPD_WHITE 1
#define GxEPD_BLACK 0

#ifndef CROWPANEL_STATIC_PREVIEW
#define CROWPANEL_STATIC_PREVIEW 0
#endif

#ifndef CROWPANEL_FRAMEBUFFER_DUMP
#define CROWPANEL_FRAMEBUFFER_DUMP 0
#endif

#ifndef CROWPANEL_FONT_GALLERY
#define CROWPANEL_FONT_GALLERY 0
#endif

enum alignment {LEFT, RIGHT, CENTER};
enum UIFontSize {UI_FONT_08, UI_FONT_10, UI_FONT_12, UI_FONT_14, UI_FONT_24};

GFXcanvas1 display(CrowPanelEPD::WIDTH, CrowPanelEPD::HEIGHT);
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

// Using fonts:
// u8g2_font_helvB08_tf
// u8g2_font_helvB10_tf
// u8g2_font_helvB12_tf
// u8g2_font_helvB14_tf
// u8g2_font_helvB18_tf
// u8g2_font_helvB24_tf

//################  VERSION  ##########################
String version = "12.6-crowpanel-green-tides";     // Version of this program
//################ VARIABLES ###########################

boolean LargeIcon = true, SmallIcon = false;
#define Large  11           // For icon drawing, needs to be odd number for best effect
#define Small  5            // For icon drawing, needs to be odd number for best effect
String  time_str, date_str; // strings to hold time and received weather data
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
long    StartTime = 0;
bool    DisplayInitialised = false;

//################ PROGRAM VARIABLES and OBJECTS ################

#define max_readings 72
#define graph_readings 24

#include <common.h>

#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false

float pressure_readings[max_readings]    = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings]    = {0};
float rain_readings[max_readings]        = {0};
float snow_readings[max_readings]        = {0};
float wind_readings[max_readings]        = {0};

long SleepDuration = 30; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupTime    = 7;  // Don't wakeup until after 07:00 to save battery power
int  SleepTime     = 23; // Sleep after (23+1) 00:00 to save battery power
int  FontGalleryIndex = 0;

void LoadStaticPreviewData();
bool ReceiveNoaaTidePredictions(bool print);
void DumpFramebuffer(const char* label);
float UnixLocalHour(int unix_time);
String ForecastTimeLabel(int unix_time);
String FormatVisibility(int visibility_meters);
void DrawNoonMidnightMarkers(int x_pos, int y_pos, int gwidth, int gheight);
void SetUIFont(UIFontSize size);
const char* FontGalleryLabel();
int FontGalleryCount();

//#########################################################################################
void setup() {
  StartTime = millis();
  Serial.begin(115200);
  CrowPanelEPD::powerOff();
#if CROWPANEL_FONT_GALLERY
  Serial.println("Running CrowPanel font gallery preview");
  LoadStaticPreviewData();
  InitialiseDisplay();
  for (FontGalleryIndex = 0; FontGalleryIndex < FontGalleryCount(); FontGalleryIndex++) {
    display.fillScreen(GxEPD_WHITE);
    DisplayWeather();
    DumpFramebuffer(FontGalleryLabel());
  }
  Serial.println("FB_GALLERY_END");
  CrowPanelEPD::powerOff();
#elif CROWPANEL_STATIC_PREVIEW
  Serial.println("Running static weather/tide panel preview");
  LoadStaticPreviewData();
  InitialiseDisplay();
  display.fillScreen(GxEPD_WHITE);
  DisplayWeather();
  DumpFramebuffer("static");
  CrowPanelEPD::refresh(display.getBuffer());
#else
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    if (CurrentHour >= WakeupTime && CurrentHour <= SleepTime ) {
      InitialiseDisplay(); // Give screen time to initialise by getting weather data!
      byte Attempts = 1;
      bool RxWeather = false;
      WiFiClient client;   // wifi client object
      while (RxWeather == false && Attempts <= 2) { // Try up-to 2 time for Weather and Forecast data
        if (RxWeather  == false) RxWeather  = ReceiveOneCallWeather(client, true);
        Attempts++;
      }
      if (RxWeather) { // Only if received both Weather or Forecast proceed
        if (!ReceiveNoaaTidePredictions(true)) {
          Serial.println("Using fallback local tide samples");
        }
        StopWiFi(); // Reduces power consumption
        display.fillScreen(GxEPD_WHITE);
        DisplayWeather();
        DumpFramebuffer("live");
        CrowPanelEPD::refresh(display.getBuffer());
      }
    }
  }
#endif
  BeginSleep();
}
//#########################################################################################
void loop() { // this will never run!
}
//#########################################################################################
void DumpFramebuffer(const char* label) {
#if CROWPANEL_FRAMEBUFFER_DUMP
  const uint8_t* buffer = display.getBuffer();
  const uint32_t bytes = ((uint32_t)CrowPanelEPD::WIDTH + 7) / 8 * CrowPanelEPD::HEIGHT;
  Serial.printf("FB_DUMP_BEGIN label=%s width=%d height=%d bytes=%lu format=GFXcanvas1 bit=1:white bit=0:black\r\n",
                label, CrowPanelEPD::WIDTH, CrowPanelEPD::HEIGHT, bytes);
  for (uint32_t index = 0; index < bytes; index++) {
    if (buffer[index] < 16) Serial.print('0');
    Serial.print(buffer[index], HEX);
    if ((index + 1) % 32 == 0) Serial.println();
  }
  if (bytes % 32 != 0) Serial.println();
  Serial.println("FB_DUMP_END");
#else
  (void)label;
#endif
}
//#########################################################################################
float UnixLocalHour(int unix_time) {
  time_t tm = unix_time + WxConditions[0].Timezone;
  struct tm *timeinfo = gmtime(&tm);
  return timeinfo->tm_hour + timeinfo->tm_min / 60.0;
}
//#########################################################################################
String ForecastTimeLabel(int unix_time) {
  time_t tm = unix_time + WxConditions[0].Timezone;
  struct tm *timeinfo = gmtime(&tm);
  char output[10];
  if (Units == "M") {
    strftime(output, sizeof(output), "%H:%M", timeinfo);
  }
  else {
    strftime(output, sizeof(output), "%I  %P", timeinfo);
  }
  return String(output);
}
//#########################################################################################
String FormatVisibility(int visibility_distance) {
  if (visibility_distance <= 0) return "NA";
  if (Units == "M") {
    if (visibility_distance >= 1000) {
      float km = visibility_distance / 1000.0;
      return String(km, km >= 10 ? 0 : 1) + "km";
    }
    return String(visibility_distance) + "m";
  }
  float miles = visibility_distance / 5280.0;
  return String(miles, miles >= 10 ? 0 : 1) + "mi";
}
//#########################################################################################
int FontGalleryCount() {
#if CROWPANEL_FONT_GALLERY
  return 10;
#else
  return 1;
#endif
}
//#########################################################################################
const char* FontGalleryLabel() {
#if CROWPANEL_FONT_GALLERY
  static const char* labels[] = {
    "helvetica_regular",
    "lucida_sans",
    "lucida_bright",
    "new_century",
    "times",
    "courier",
    "x11_9x15",
    "t0_terminal",
    "profont",
    "fub"
  };
  return labels[FontGalleryIndex];
#else
  return "normal";
#endif
}
//#########################################################################################
const uint8_t* FontGalleryFont(UIFontSize size) {
#if CROWPANEL_FONT_GALLERY
  switch (FontGalleryIndex) {
    case 0:
      if (size == UI_FONT_08) return u8g2_font_helvR08_tf;
      if (size == UI_FONT_10) return u8g2_font_helvR10_tf;
      if (size == UI_FONT_12) return u8g2_font_helvR12_tf;
      if (size == UI_FONT_14) return u8g2_font_helvR14_tf;
      return u8g2_font_helvR24_tf;
    case 1:
      if (size == UI_FONT_08) return u8g2_font_luRS08_tf;
      if (size == UI_FONT_10) return u8g2_font_luRS10_tf;
      if (size == UI_FONT_12) return u8g2_font_luRS12_tf;
      if (size == UI_FONT_14) return u8g2_font_luRS14_tf;
      return u8g2_font_luRS24_tf;
    case 2:
      if (size == UI_FONT_08) return u8g2_font_lubR08_tf;
      if (size == UI_FONT_10) return u8g2_font_lubR10_tf;
      if (size == UI_FONT_12) return u8g2_font_lubR12_tf;
      if (size == UI_FONT_14) return u8g2_font_lubR14_tf;
      return u8g2_font_lubR24_tf;
    case 3:
      if (size == UI_FONT_08) return u8g2_font_ncenR08_tf;
      if (size == UI_FONT_10) return u8g2_font_ncenR10_tf;
      if (size == UI_FONT_12) return u8g2_font_ncenR12_tf;
      if (size == UI_FONT_14) return u8g2_font_ncenR14_tf;
      return u8g2_font_ncenR24_tf;
    case 4:
      if (size == UI_FONT_08) return u8g2_font_timR08_tf;
      if (size == UI_FONT_10) return u8g2_font_timR10_tf;
      if (size == UI_FONT_12) return u8g2_font_timR12_tf;
      if (size == UI_FONT_14) return u8g2_font_timR14_tf;
      return u8g2_font_timR24_tf;
    case 5:
      if (size == UI_FONT_08) return u8g2_font_courR08_tf;
      if (size == UI_FONT_10) return u8g2_font_courR10_tf;
      if (size == UI_FONT_12) return u8g2_font_courR12_tf;
      if (size == UI_FONT_14) return u8g2_font_courR14_tf;
      return u8g2_font_courR24_tf;
    case 6:
      if (size == UI_FONT_08) return u8g2_font_7x13_tf;
      return u8g2_font_9x15_tf;
    case 7:
      if (size == UI_FONT_08) return u8g2_font_t0_11_tf;
      if (size == UI_FONT_10) return u8g2_font_t0_12_tf;
      if (size == UI_FONT_12) return u8g2_font_t0_14_tf;
      if (size == UI_FONT_14) return u8g2_font_t0_17_tf;
      return u8g2_font_t0_22_tf;
    case 8:
      if (size == UI_FONT_08) return u8g2_font_profont10_tf;
      if (size == UI_FONT_10) return u8g2_font_profont11_tf;
      if (size == UI_FONT_12) return u8g2_font_profont12_tf;
      if (size == UI_FONT_14) return u8g2_font_profont17_tf;
      return u8g2_font_profont22_tf;
    case 9:
      if (size == UI_FONT_08) return u8g2_font_fub11_tf;
      if (size == UI_FONT_10) return u8g2_font_fub11_tf;
      if (size == UI_FONT_12) return u8g2_font_fub14_tf;
      if (size == UI_FONT_14) return u8g2_font_fub17_tf;
      return u8g2_font_fub25_tf;
  }
#endif
  if (size == UI_FONT_08) return u8g2_font_profont11_tf;
  if (size == UI_FONT_10) return u8g2_font_profont12_tf;
  if (size == UI_FONT_12) return u8g2_font_profont15_tf;
  if (size == UI_FONT_14) return u8g2_font_profont22_tf;
  return u8g2_font_profont29_tf;
}
//#########################################################################################
void SetUIFont(UIFontSize size) {
  u8g2Fonts.setFont(FontGalleryFont(size));
}
//#########################################################################################
bool ReceiveNoaaTidePredictions(bool print) {
  Serial.println("Rx NOAA tide predictions...");
  WiFiClientSecure secure_client;
  secure_client.setInsecure();
  HTTPClient http;
  String uri = "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?date=today&station=" +
               String(NOAA_TIDE_STATION) +
               "&product=predictions&datum=MLLW&time_zone=lst_ldt&units=english&interval=60&format=json";
  http.begin(secure_client, uri);
  http.useHTTP10(true);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("NOAA tide HTTP status: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    String response = http.getString();
    if (response.length() > 0) Serial.println(response.substring(0, 240));
    http.end();
    return false;
  }

  JsonDocument doc;
  String response = http.getString();
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("NOAA tide deserializeJson() failed: ");
    Serial.println(error.c_str());
    Serial.println(response.substring(0, 120));
    http.end();
    return false;
  }

  JsonArray predictions = doc["predictions"];
  if (predictions.isNull() || predictions.size() == 0) {
    Serial.println("NOAA tide response contained no predictions");
    http.end();
    return false;
  }

  int count = 0;
  for (JsonObject prediction : predictions) {
    if (count >= LocalTideSampleMax - 1) break;
    String t = prediction["t"] | "";
    if (t.length() < 16) continue;
    int hour = t.substring(11, 13).toInt();
    int minute = t.substring(14, 16).toInt();
    LocalTideSamples[count].hour = hour + minute / 60.0;
    LocalTideSamples[count].height_ft = String(prediction["v"] | "0").toFloat();
    count++;
  }

  if (count > 0) {
    LocalTideSampleCount = count;
    if (LocalTideSamples[count - 1].hour < 24 && count < LocalTideSampleMax) {
      LocalTideSamples[count].hour = 24;
      LocalTideSamples[count].height_ft = LocalTideSamples[count - 1].height_ft;
      LocalTideSampleCount++;
    }
  }

  http.end();
  if (print) {
    Serial.println("NOAA tide station: " + String(NOAA_TIDE_STATION_NAME));
    Serial.println("NOAA tide samples: " + String(LocalTideSampleCount));
  }
  return LocalTideSampleCount > 0;
}
//#########################################################################################
void LoadStaticPreviewData() {
  const bool metric = Units == "M";
  CurrentHour = 14;
  CurrentMin = 35;
  CurrentSec = 0;
  wifi_signal = -54;
  date_str = "Thu Jun-11-2026";
  time_str = metric ? "14:35:00" : "02:35pm";

  WxConditions[0].Timezone = 0;
  WxConditions[0].Sunrise = 6 * 3600 + 8 * 60;
  WxConditions[0].Sunset = 20 * 3600 + 18 * 60;
  WxConditions[0].Temperature = metric ? 20.4 : 68.7;
  WxConditions[0].High = metric ? 23.0 : 73.0;
  WxConditions[0].Low = metric ? 14.0 : 57.0;
  WxConditions[0].Humidity = 64;
  WxConditions[0].Pressure = metric ? 1017 : 30.0;
  WxConditions[0].Windspeed = metric ? 4.8 : 10.7;
  WxConditions[0].Winddir = 145;
  WxConditions[0].Rainfall = 0;
  WxConditions[0].Snowfall = 0;
  WxConditions[0].Cloudcover = 38;
  WxConditions[0].Visibility = 10000;
  WxConditions[0].Icon = "02d";
  WxConditions[0].Description = "partly cloudy";
  WxConditions[0].Trend = "0";

  const char* icons[] = {"02d", "01d", "03d", "10d", "04d", "01n", "02n", "03n"};
  for (int r = 0; r < 8; r++) {
    Daily[r].Dt = (15 + r * 3) * 3600;
    Daily[r].Icon = icons[r % 8];
    Daily[r].High = (metric ? 21 : 70) + (r % 3);
    Daily[r].Low = (metric ? 13 : 56) + (r % 2);
    Daily[r].Description = "Preview forecast";
  }

  for (int r = 0; r < max_readings; r++) {
    const float daily_wave = sin((r + CurrentHour - 8) * PI / 12.0);
    const float gust_wave = sin(r * PI / 9.0);
    WxForecast[r].Dt = (CurrentHour + r) * 3600;
    WxForecast[r].Temperature = (metric ? 18.0 : 64.0) + daily_wave * (metric ? 4.0 : 7.0) + (r / 24) * (metric ? 0.6 : 1.0);
    WxForecast[r].FeelsLike = WxForecast[r].Temperature - (metric ? 1.0 : 2.0);
    WxForecast[r].Pressure = metric ? (1016 + cos(r * PI / 24.0) * 4) : (30.0 + cos(r * PI / 24.0) * 0.12);
    WxForecast[r].Humidity = 58 + r % 7;
    WxForecast[r].Windspeed = (metric ? 4.0 : 9.0) + abs(gust_wave) * (metric ? 4.0 : 8.0);
    WxForecast[r].Winddir = 120 + r * 8;
    WxForecast[r].Rainfall = (r == 9 || r == 10 || r == 18) ? (metric ? 0.8 : 0.03) : 0;
    WxForecast[r].Snowfall = 0;
    WxForecast[r].Icon = icons[r % 8];
  }
}
//#########################################################################################
void BeginSleep() {
  if (DisplayInitialised) CrowPanelEPD::sleep();
  CrowPanelEPD::powerOff();
  long SleepTimer = SleepDuration * 60; // theoretical sleep duration
  long offset = (CurrentMin % SleepDuration) * 60 + CurrentSec; // number of seconds elapsed after last theoretical wake-up time point
  if (offset > SleepDuration/2 * 60){ // waking up too early will cause <offset> too large
    offset -= SleepDuration * 60; // then we should make it negative, so as to extend this coming sleep duration
  }
  esp_sleep_enable_timer_wakeup((SleepTimer - offset) * 1000000LL); // do compensation to cover ESP32 RTC timer source inaccuracies
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
  digitalWrite(BUILTIN_LED, HIGH);
#endif
  Serial.println("Entering " + String(SleepTimer) + "-secs of sleep time");
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();      // Sleep for e.g. 30 minutes
}
//#########################################################################################
void DisplayWeather() {                 // 4.2" e-paper display is 400x300 resolution
  DrawHeadingSection();                 // Top line of the display
  DrawMainWeatherSection(172, 70);      // Centre section of display for Location, temperature, Weather report, current Wx Symbol and wind direction
  DrawForecastSection(233, 15);         // 3hr forecast boxes
  DisplayPrecipitationSection(233, 82); // Current precip, visibility and cloud cover
  DrawAstronomySection(233, 74);        // Middle-right Sun rise/set, Moon phase and Moon icon
}
//#########################################################################################
void DrawHeadingSection() {
  SetUIFont(UI_FONT_08);
  drawString(SCREEN_WIDTH / 2, 0, City, CENTER);
  drawString(SCREEN_WIDTH, 0, date_str, RIGHT);
  drawString(4, 0, time_str, LEFT);
  DrawBattery(65, 12);
  display.drawLine(0, 12, SCREEN_WIDTH, 12, GxEPD_BLACK);
}
//#########################################################################################
void DrawMainWeatherSection(int x, int y) {
  DisplayDisplayWindSection(x - 115, y - 3, WxConditions[0].Winddir, WxConditions[0].Windspeed, 40);
  DisplayWXicon(x + 5, y - 5, WxConditions[0].Icon, LargeIcon);
  SetUIFont(UI_FONT_10);
  DrawCurrentTideStatus(x - 120, y + 58);
  SetUIFont(UI_FONT_12);
  DrawMainWx(x, y + 60);
  display.drawRect(0, y + 68, 232, 48, GxEPD_BLACK);
  DrawTide24hGraph(0, y + 68, 232, 48);
}
//#########################################################################################
void DrawForecastSection(int x, int y) {
  SetUIFont(UI_FONT_10);
  DrawForecastWeather(x, y, 0);
  DrawForecastWeather(x + 56, y, 1);
  DrawForecastWeather(x + 112, y, 2);
  //       (x,y,width,height,MinValue, MaxValue, Title, Data Array, AutoScale, ChartMode)
  for (int r = 0; r < graph_readings; r++) {
#if CROWPANEL_USE_OPEN_METEO
    int start = r * 3;
    int end = min(start + 3, max_readings);
    float temp_sum = 0;
    float wind_sum = 0;
    float pressure_sum = 0;
    float rain_sum = 0;
    int count = 0;
    for (int sample = start; sample < end; sample++) {
      temp_sum += WxForecast[sample].Temperature;
      wind_sum += WxForecast[sample].Windspeed;
      pressure_sum += WxForecast[sample].Pressure;
      rain_sum += WxForecast[sample].Rainfall;
      count++;
    }
    if (count == 0) count = 1;
    temperature_readings[r] = temp_sum / count;
    wind_readings[r]        = wind_sum / count;
    pressure_readings[r]    = pressure_sum / count;
    rain_readings[r]        = rain_sum;
#else
    if (Units == "I") {
      pressure_readings[r] = WxForecast[r].Pressure * 0.02953;
      rain_readings[r]     = WxForecast[r].Rainfall * 0.0393701;
    }
    else {
      pressure_readings[r] = WxForecast[r].Pressure;
      rain_readings[r]     = WxForecast[r].Rainfall;
    }
    temperature_readings[r] = WxForecast[r].Temperature;
    wind_readings[r]        = WxForecast[r].Windspeed;
#endif
  }
  display.drawLine(0, y + 172, SCREEN_WIDTH, y + 172, GxEPD_BLACK);
  SetUIFont(UI_FONT_10);
  DrawGraph(SCREEN_WIDTH / 400 * 30,  SCREEN_HEIGHT / 300 * 206, SCREEN_WIDTH / 4, SCREEN_HEIGHT / 5 + 10, 0, 30, Units == "M" ? "Wind (m/s)" : "Wind (mph)", wind_readings, graph_readings, autoscale_on, barchart_off);
  DrawGraph(SCREEN_WIDTH / 400 * 158, SCREEN_HEIGHT / 300 * 206, SCREEN_WIDTH / 4, SCREEN_HEIGHT / 5 + 10, 10, 30, Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_readings, graph_readings, autoscale_on, barchart_off);
  DrawGraph(SCREEN_WIDTH / 400 * 288, SCREEN_HEIGHT / 300 * 206, SCREEN_WIDTH / 4, SCREEN_HEIGHT / 5 + 10, 0, 30, Units == "M" ? TXT_RAINFALL_MM : TXT_RAINFALL_IN, rain_readings, graph_readings, autoscale_on, barchart_on);
}
//#########################################################################################
void DrawForecastWeather(int x, int y, int index) {
  const int forecast_index = min((index + 1) * 3, max_readings - 1);
  SetUIFont(UI_FONT_08);
  display.drawRect(x, y, 55, 65, GxEPD_BLACK);
  display.drawLine(x + 1, y + 13, x + 54, y + 13, GxEPD_BLACK);
  DisplayWXicon(x + 28, y + 35, WxForecast[forecast_index].Icon, SmallIcon);
  drawString(x + 31, y + 3, ForecastTimeLabel(WxForecast[forecast_index].Dt), CENTER);
  const int temp_y = y + 52;
  const int temp_center_x = x + 28;
  const String left_temp = String(WxForecast[forecast_index].Temperature, 0) + "°";
  const String separator = " / ";
  const String right_temp = String(WxForecast[forecast_index].FeelsLike, 0) + "°";
  const int left_width = u8g2Fonts.getUTF8Width(left_temp.c_str());
  const int separator_width = u8g2Fonts.getUTF8Width(separator.c_str());
  const int right_width = u8g2Fonts.getUTF8Width(right_temp.c_str());
  const int temp_left = temp_center_x - (left_width + separator_width + right_width) / 2;
  drawString(temp_left + 2, temp_y, left_temp, LEFT);
  drawString(temp_left + left_width, temp_y, separator, LEFT);
  drawString(temp_left + left_width + separator_width - 2, temp_y, right_temp, LEFT);
}
//#########################################################################################
void DrawMainWx(int x, int y) {
  SetUIFont(UI_FONT_14);
  drawString(x + 5, y - 22, String(WxConditions[0].Temperature, 0) + "°" + (Units == "M" ? "C" : "F"), CENTER); // Show current Temperature
  SetUIFont(UI_FONT_12);
  drawString(x + 5, y - 3, String(WxConditions[0].High, 0) + "° | " + String(WxConditions[0].Low, 0) + "°", CENTER); // Show forecast high and Low
}
//#########################################################################################
void DisplayDisplayWindSection(int x, int y, float angle, float windspeed, int Cradius) {
  arrow(x, y, int(Cradius * 0.7) + 2, angle, 9, Cradius - 2); // Show wind direction between the compass rings
  SetUIFont(UI_FONT_08);
  int dxo, dyo, dxi, dyi;
  display.drawLine(0, 15, 0, y + Cradius + 30, GxEPD_BLACK);
  display.drawCircle(x, y, Cradius, GxEPD_BLACK);     // Draw compass circle
  display.drawCircle(x, y, Cradius + 1, GxEPD_BLACK); // Draw compass circle
  display.drawCircle(x, y, Cradius * 0.7, GxEPD_BLACK); // Draw compass inner circle
  for (float a = 0; a < 360; a = a + 22.5) {
    dxo = Cradius * cos((a - 90) * PI / 180);
    dyo = Cradius * sin((a - 90) * PI / 180);
    if (a == 45)  drawString(dxo + x + 10, dyo + y - 10, TXT_NE, CENTER);
    if (a == 135) drawString(dxo + x + 7,  dyo + y + 5,  TXT_SE, CENTER);
    if (a == 225) drawString(dxo + x - 15, dyo + y,      TXT_SW, CENTER);
    if (a == 315) drawString(dxo + x - 15, dyo + y - 10, TXT_NW, CENTER);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    display.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, GxEPD_BLACK);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    display.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, GxEPD_BLACK);
  }
  drawString(x, y - Cradius - 10,     TXT_N, CENTER);
  drawString(x, y + Cradius + 5,      TXT_S, CENTER);
  drawString(x - Cradius - 10, y - 3, TXT_W, CENTER);
  drawString(x + Cradius + 8,  y - 3, TXT_E, CENTER);
  drawString(x - 2, y - 13, WindDegToDirection(angle), CENTER);
  drawString(x + 3, y + 3, String(windspeed, 0) + (Units == "M" ? "m/s" : "mph"), CENTER);
}
//#########################################################################################
String WindDegToDirection(float winddirection) {
  int dir = int((winddirection / 22.5) + 0.5);
  String Ord_direction[16] = {TXT_N, TXT_NNE, TXT_NE, TXT_ENE, TXT_E, TXT_ESE, TXT_SE, TXT_SSE, TXT_S, TXT_SSW, TXT_SW, TXT_WSW, TXT_W, TXT_WNW, TXT_NW, TXT_NNW};
  return Ord_direction[(dir % 16)];
}
//#########################################################################################
void DrawPressureAndTrend(int x, int y, float pressure, String slope) {
  drawString(x, y, String(pressure, (Units == "M" ? 0 : 1)) + (Units == "M" ? "mb" : "in"), CENTER);
  x = x + 40; y = y + 2;
  if      (slope == "+") {
    display.drawLine(x,  y, x + 4, y - 4, GxEPD_BLACK);
    display.drawLine(x + 4, y - 4, x + 8, y, GxEPD_BLACK);
  }
  else if (slope == "0") {
    display.drawLine(x + 4, y - 4, x + 8, y, GxEPD_BLACK);
    display.drawLine(x + 4, y + 4, x + 8, y, GxEPD_BLACK);
  }
  else if (slope == "-") {
    display.drawLine(x,  y, x + 4, y + 4, GxEPD_BLACK);
    display.drawLine(x + 4, y + 4, x + 8, y, GxEPD_BLACK);
  }
}
//#########################################################################################
float TideHeightAtHour(float hour) {
  if (LocalTideSampleCount == 0) return 0;
  if (hour <= LocalTideSamples[0].hour) return LocalTideSamples[0].height_ft;
  for (int i = 1; i < LocalTideSampleCount; i++) {
    if (hour <= LocalTideSamples[i].hour) {
      float prev_hour = LocalTideSamples[i - 1].hour;
      float next_hour = LocalTideSamples[i].hour;
      float span = next_hour - prev_hour;
      if (span <= 0) return LocalTideSamples[i].height_ft;
      float ratio = (hour - prev_hour) / span;
      return LocalTideSamples[i - 1].height_ft + ratio * (LocalTideSamples[i].height_ft - LocalTideSamples[i - 1].height_ft);
    }
  }
  return LocalTideSamples[LocalTideSampleCount - 1].height_ft;
}
//#########################################################################################
bool IsTideRisingAtHour(float hour) {
  float next_hour = hour + 0.5;
  if (next_hour > 24) next_hour = 24;
  return TideHeightAtHour(next_hour) >= TideHeightAtHour(hour);
}
//#########################################################################################
void DrawTideArrow(int x, int y, bool rising) {
  if (rising) {
    display.fillTriangle(x, y - 6, x - 4, y + 1, x + 4, y + 1, GxEPD_BLACK);
    display.fillRect(x - 1, y + 1, 3, 6, GxEPD_BLACK);
  }
  else {
    display.fillTriangle(x, y + 6, x - 4, y - 1, x + 4, y - 1, GxEPD_BLACK);
    display.fillRect(x - 1, y - 7, 3, 6, GxEPD_BLACK);
  }
}
//#########################################################################################
void DrawCurrentTideStatus(int x, int y) {
  float current_hour = CurrentHour + CurrentMin / 60.0;
  float tide_height = TideHeightAtHour(current_hour);
  bool rising = IsTideRisingAtHour(current_hour);
  SetUIFont(UI_FONT_12);
  drawString(x - 4, y - 2, String(tide_height, 1) + "ft", CENTER);
  DrawTideArrow(x + 36, y, rising);
}
//#########################################################################################
void DrawTide24hGraph(int x, int y, int w, int h) {
  if (LocalTideSampleCount == 0) {
    SetUIFont(UI_FONT_08);
    drawString(x + w / 2, y + h / 2 - 5, "No tide data", CENTER);
    return;
  }

  float min_height = LocalTideSamples[0].height_ft;
  float max_height = LocalTideSamples[0].height_ft;
  for (int i = 1; i < LocalTideSampleCount; i++) {
    if (LocalTideSamples[i].height_ft < min_height) min_height = LocalTideSamples[i].height_ft;
    if (LocalTideSamples[i].height_ft > max_height) max_height = LocalTideSamples[i].height_ft;
  }
  if (max_height - min_height < 0.1) {
    max_height += 0.1;
    min_height -= 0.1;
  }

  const int graph_x = x + 8;
  const int graph_y = y + 13;
  const int graph_w = w - 16;
  const int graph_h = h - 24;

  SetUIFont(UI_FONT_08);
  drawString(x + 6, y + 2, "24h Tide", LEFT);
  drawString(x + w - 6, y + 2, String(max_height, 1) + "ft", RIGHT);
  display.drawRect(graph_x, graph_y, graph_w, graph_h, GxEPD_BLACK);

  int last_x = graph_x;
  int last_y = graph_y + graph_h / 2;
  for (int hour = 0; hour <= 24; hour++) {
    float tide_height = TideHeightAtHour(hour);
    int px = graph_x + hour * graph_w / 24;
    int py = graph_y + graph_h - 2 - int((tide_height - min_height) * (graph_h - 4) / (max_height - min_height));
    if (hour > 0) display.drawLine(last_x, last_y, px, py, GxEPD_BLACK);
    last_x = px;
    last_y = py;
  }

  const float sun_hours[] = {UnixLocalHour(WxConditions[0].Sunrise), UnixLocalHour(WxConditions[0].Sunset)};
  for (int marker = 0; marker < 2; marker++) {
    int sx = graph_x + int(sun_hours[marker] * graph_w / 24.0);
    for (int sy = graph_y + 1; sy < graph_y + graph_h; sy += 4) {
      display.drawFastVLine(sx, sy, 2, GxEPD_BLACK);
    }
  }

  for (int hour = 0; hour < 24; hour += 6) {
    int tx = graph_x + hour * graph_w / 24;
    display.drawLine(tx, graph_y + graph_h, tx, graph_y + graph_h + 3, GxEPD_BLACK);
    String label = String(hour) + ":00";
    drawString(tx, graph_y + graph_h + 4, label, hour == 0 ? LEFT : CENTER);
  }

  int now_x = graph_x + int((CurrentHour + CurrentMin / 60.0) * graph_w / 24.0);
  display.drawLine(now_x, graph_y, now_x, graph_y + graph_h, GxEPD_BLACK);
}
//#########################################################################################
void DisplayPrecipitationSection(int x, int y) {
  display.drawRect(x, y - 1, 167, 56, GxEPD_BLACK);
  SetUIFont(UI_FONT_12);

  const float precip = WxForecast[1].Rainfall;
  const String precip_units = Units == "M" ? "mm" : "in";
  const String precip_text = String(precip, precip >= 1 ? 1 : 2) + precip_units;
  const int left_icon_x = x + 17;
  const int left_text_x = x + 33;
  const int right_icon_x = x + 104;
  const int right_text_x = x + 119;
  const int top_icon_y = y + 14;
  const int top_text_y = y + 8;
  const int bottom_icon_y = y + 39;
  const int bottom_text_y = y + 33;

  addraindrop(left_icon_x - 3, top_icon_y + 1, 4);
  drawString(left_text_x, top_text_y, precip_text, LEFT);

  Visibility(right_icon_x, top_icon_y + 1, FormatVisibility(WxConditions[0].Visibility), right_text_x, top_text_y);

  Humidity(left_icon_x, bottom_icon_y);
  drawString(left_text_x, bottom_text_y, String(WxConditions[0].Humidity, 0) + "%", LEFT);

  CloudCover(right_icon_x, bottom_icon_y + 1, WxConditions[0].Cloudcover, right_text_x, bottom_text_y);
}
void DrawAstronomySection(int x, int y) {
  SetUIFont(UI_FONT_08);
  display.drawRect(x, y + 64, 167, 48, GxEPD_BLACK);
  drawString(x + 7, y + 70, ConvertUnixTime(WxConditions[0].Sunrise + WxConditions[0].Timezone).substring(0, (Units == "M" ? 5 : 7)) + " " + TXT_SUNRISE, LEFT);
  drawString(x + 7, y + 85, ConvertUnixTime(WxConditions[0].Sunset + WxConditions[0].Timezone).substring(0, (Units == "M" ? 5 : 7)) + " " + TXT_SUNSET, LEFT);
  time_t now = time(NULL);
  struct tm * now_utc = gmtime(&now);
  const int day_utc   = now_utc->tm_mday;
  const int month_utc = now_utc->tm_mon + 1;
  const int year_utc  = now_utc->tm_year + 1900;
  drawString(x + 7, y + 100, MoonPhase(day_utc, month_utc, year_utc), LEFT);
  DrawMoon(x + 105, y + 50, day_utc, month_utc, year_utc, Hemisphere);
}
//#########################################################################################
void DrawMoon(int x, int y, int dd, int mm, int yy, String hemisphere) {
  const int diameter = 38;
  double Phase = NormalizedMoonPhase(dd, mm, yy);
  hemisphere.toLowerCase();
  if (hemisphere == "south") Phase = 1 - Phase;
  // Draw dark part of moon
  display.fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, GxEPD_BLACK);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= 45; Ypos++) {
    double Xpos = sqrt(45 * 45 - Ypos * Ypos);
    // Determine the edges of the lighted part of the moon
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (Phase < 0.5) {
      Xpos1 = - Xpos;
      Xpos2 = (Rpos - 2 * Phase * Rpos - Xpos);
    }
    else {
      Xpos1 = Xpos;
      Xpos2 = (Xpos - 2 * Phase * Rpos + Rpos);
    }
    // Draw light part of moon
    double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    display.drawLine(pW1x, pW1y, pW2x, pW2y, GxEPD_WHITE);
    display.drawLine(pW3x, pW3y, pW4x, pW4y, GxEPD_WHITE);
  }
  display.drawCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, GxEPD_BLACK);
}
//#########################################################################################
String MoonPhase(int d, int m, int y) {
  int c, e;
  double jd;
  int b;
  if (m < 3) {
    y--;
    m += 12;
  }
  ++m;
  c   = 365.25 * y;
  e   = 30.6  * m;
  jd  = c + e + d - 694039.09;     /* jd is total days elapsed */
  jd /= 29.53059;                  /* divide by the moon cycle (29.53 days) */
  b   = jd;                        /* int(jd) -> b, take integer part of jd */
  jd -= b;                         /* subtract integer part to leave fractional part of original jd */
  b   = jd * 8 + 0.5;              /* scale fraction from 0-8 and round by adding 0.5 */
  b   = b & 7;                     /* 0 and 8 are the same phase so modulo 8 for 0 */
  Hemisphere.toLowerCase();
  if (Hemisphere == "south") b = 7 - b;
  if (b == 0) return TXT_MOON_NEW;              // New;              0%  illuminated
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;  // Waxing crescent; 25%  illuminated
  if (b == 2) return TXT_MOON_FIRST_QUARTER;    // First quarter;   50%  illuminated
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;   // Waxing gibbous;  75%  illuminated
  if (b == 4) return TXT_MOON_FULL;             // Full;            100% illuminated
  if (b == 5) return TXT_MOON_WANING_GIBBOUS;   // Waning gibbous;  75%  illuminated
  if (b == 6) return TXT_MOON_THIRD_QUARTER;    // Third quarter;   50%  illuminated
  if (b == 7) return TXT_MOON_WANING_CRESCENT;  // Waning crescent; 25%  illuminated
  return "";
}
//#########################################################################################
void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  const float angle = aangle * PI / 180.0;
  const float ux = sin(angle);
  const float uy = -cos(angle);
  const float px = cos(angle);
  const float py = sin(angle);
  const float outer_radius = plength;
  const float inner_radius = asize;
  const float half_width = pwidth / 2.0;

  const int tip_x = round(x + ux * outer_radius);
  const int tip_y = round(y + uy * outer_radius);
  const float base_x = x + ux * inner_radius;
  const float base_y = y + uy * inner_radius;
  const int base_left_x = round(base_x + px * half_width);
  const int base_left_y = round(base_y + py * half_width);
  const int base_right_x = round(base_x - px * half_width);
  const int base_right_y = round(base_y - py * half_width);

  display.fillTriangle(tip_x, tip_y, base_left_x, base_left_y, base_right_x, base_right_y, GxEPD_BLACK);
}
//#########################################################################################
void DisplayWXicon(int x, int y, String IconName, bool IconSize) {
  Serial.println(IconName);
  if      (IconName == "01d" || IconName == "01n")  Sunny(x, y, IconSize, IconName);
  else if (IconName == "02d" || IconName == "02n")  MostlySunny(x, y, IconSize, IconName);
  else if (IconName == "03d" || IconName == "03n")  Cloudy(x, y, IconSize, IconName);
  else if (IconName == "04d" || IconName == "04n")  MostlyCloudy(x, y, IconSize, IconName);
  else if (IconName == "09d" || IconName == "09n")  ChanceRain(x, y, IconSize, IconName);
  else if (IconName == "10d" || IconName == "10n")  Rain(x, y, IconSize, IconName);
  else if (IconName == "11d" || IconName == "11n")  Tstorms(x, y, IconSize, IconName);
  else if (IconName == "13d" || IconName == "13n")  Snow(x, y, IconSize, IconName);
  else if (IconName == "50d")                       Haze(x, y, IconSize, IconName);
  else if (IconName == "50n")                       Fog(x, y, IconSize, IconName);
  else                                              Nodata(x, y, IconSize, IconName);
}
//#########################################################################################
uint8_t StartWiFi() {
  Serial.print("\r\nConnecting to: "); Serial.println(String(ssid));
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection) {
    connectionStatus = WiFi.status();
    if (millis() > start + 15000) { // Wait 15-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else Serial.println("WiFi connection *** FAILED ***");
  return connectionStatus;
}
//#########################################################################################
void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}
//#########################################################################################
boolean SetupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  bool TimeStatus = UpdateLocalTime();
  return TimeStatus;
}
//#########################################################################################
boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 10000)) { // Wait for 5-sec for time to synchronise
    Serial.println("Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  //Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S"); // Displays: Saturday, June 24 2017 14:05:49
  if (Units == "M") {
    if ((Language == "CZ") || (Language == "DE") || (Language == "NL") || (Language == "PL")) {
      sprintf(day_output, "%s, %02u. %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900); // day_output >> So., 23. Juni 2019 <<
    }
    else
    {
      sprintf(day_output, "%s  %02u-%s-%04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    }
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a  %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);         // Creates: '@ 02:05:49pm'
    sprintf(time_output, "%s", update_time);
  }
  date_str = day_output;
  time_str = time_output;
  return true;
}
//#########################################################################################
// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale, int linesize) {
  //Draw cloud outer
  display.fillCircle(x - scale * 3, y, scale, GxEPD_BLACK);                      // Left most circle
  display.fillCircle(x + scale * 3, y, scale, GxEPD_BLACK);                      // Right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4, GxEPD_BLACK);            // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, GxEPD_BLACK); // Right middle upper circle
  display.fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, GxEPD_BLACK); // Upper and lower lines
  //Clear cloud inner
  display.fillCircle(x - scale * 3, y, scale - linesize, GxEPD_WHITE);           // Clear left most circle
  display.fillCircle(x + scale * 3, y, scale - linesize, GxEPD_WHITE);           // Clear right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, GxEPD_WHITE); // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, GxEPD_WHITE); // Right middle upper circle
  display.fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, GxEPD_WHITE); // Upper and lower lines
}
//#########################################################################################
void addraindrop(int x, int y, int scale) {
  display.fillCircle(x, y, scale / 2, GxEPD_BLACK);
  display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y , GxEPD_BLACK);
  x = x + scale * 1.6; y = y + scale / 3;
  display.fillCircle(x, y, scale / 2, GxEPD_BLACK);
  display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y , GxEPD_BLACK);
}
//#########################################################################################
void addrain(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) scale *= 1.34;
  for (int d = 0; d < 4; d++) {
    addraindrop(x + scale * (7.8 - d * 1.95) - scale * 5.2, y + scale * 2.1 - scale / 6, scale / 1.6);
  }
}
//#########################################################################################
void addsnow(int x, int y, int scale, bool IconSize) {
  int dxo, dyo, dxi, dyi;
  for (int flakes = 0; flakes < 5; flakes++) {
    for (int i = 0; i < 360; i = i + 45) {
      dxo = 0.5 * scale * cos((i - 90) * 3.14 / 180); dxi = dxo * 0.1;
      dyo = 0.5 * scale * sin((i - 90) * 3.14 / 180); dyi = dyo * 0.1;
      display.drawLine(dxo + x + flakes * 1.5 * scale - scale * 3, dyo + y + scale * 2, dxi + x + 0 + flakes * 1.5 * scale - scale * 3, dyi + y + scale * 2, GxEPD_BLACK);
    }
  }
}
//#########################################################################################
void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 5; i++) {
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, GxEPD_BLACK);
    if (scale != Small) {
      display.drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, GxEPD_BLACK);
      display.drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, GxEPD_BLACK);
    }
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, GxEPD_BLACK);
    if (scale != Small) {
      display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, GxEPD_BLACK);
      display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, GxEPD_BLACK);
    }
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, GxEPD_BLACK);
    if (scale != Small) {
      display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, GxEPD_BLACK);
      display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, GxEPD_BLACK);
    }
  }
}
//#########################################################################################
void addsun(int x, int y, int scale, bool IconSize) {
  int linesize = 3;
  if (IconSize == SmallIcon) linesize = 1;
  display.fillRect(x - scale * 2, y, scale * 4, linesize, GxEPD_BLACK);
  display.fillRect(x, y - scale * 2, linesize, scale * 4, GxEPD_BLACK);
  display.drawLine(x - scale * 1.3, y - scale * 1.3, x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
  display.drawLine(x - scale * 1.3, y + scale * 1.3, x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
  if (IconSize == LargeIcon) {
    display.drawLine(1 + x - scale * 1.3, y - scale * 1.3, 1 + x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
    display.drawLine(2 + x - scale * 1.3, y - scale * 1.3, 2 + x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
    display.drawLine(3 + x - scale * 1.3, y - scale * 1.3, 3 + x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
    display.drawLine(1 + x - scale * 1.3, y + scale * 1.3, 1 + x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
    display.drawLine(2 + x - scale * 1.3, y + scale * 1.3, 2 + x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
    display.drawLine(3 + x - scale * 1.3, y + scale * 1.3, 3 + x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
  }
  display.fillCircle(x, y, scale * 1.3, GxEPD_WHITE);
  display.fillCircle(x, y, scale, GxEPD_BLACK);
  display.fillCircle(x, y, scale - linesize, GxEPD_WHITE);
}
//#########################################################################################
void addfog(int x, int y, int scale, int linesize, bool IconSize) {
  if (IconSize == SmallIcon) {
    y -= 10;
    linesize = 1;
  }
  for (int i = 0; i < 6; i++) {
    display.fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, GxEPD_BLACK);
    display.fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, GxEPD_BLACK);
    display.fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, GxEPD_BLACK);
  }
}
//#########################################################################################
void Sunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small, offset = 3;
  if (IconSize == LargeIcon) {
    scale = Large;
    y = y - 8;
    offset = 18;
  } else y = y - 3; // Shift up small sun icon
  if (IconName.endsWith("n")) addmoon(x, y + offset, scale, IconSize);
  scale = scale * 1.6;
  addsun(x, y, scale, IconSize);
}
//#########################################################################################
void MostlySunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 3, offset = 3;
  if (IconSize == LargeIcon) {
    scale = Large;
    offset = 10;
  } else linesize = 1;
  if (IconName.endsWith("n")) addmoon(x, y + offset, scale, IconSize);
  addcloud(x, y + offset, scale, linesize);
  addsun(x - scale * 1.8, y - scale * 1.8 + offset, scale, IconSize);
}
//#########################################################################################
void MostlyCloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
}
//#########################################################################################
void Cloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    linesize = 1;
    addcloud(x, y, scale, linesize);
  }
  else {
    y += 10;
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addcloud(x + 30, y - 35, 5, linesize); // Cloud top right
    addcloud(x - 20, y - 25, 7, linesize); // Cloud top left
    addcloud(x, y, scale, linesize);       // Main cloud
  }
}
//#########################################################################################
void Rain(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y + 10, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}
//#########################################################################################
void ExpectRain(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}
//#########################################################################################
void ChanceRain(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}
//#########################################################################################
void Tstorms(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addtstorm(x, y, scale);
}
//#########################################################################################
void Snow(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y + 15, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addsnow(x, y, scale, IconSize);
}
//#########################################################################################
void Fog(int x, int y, bool IconSize, String IconName) {
  int linesize = 3, scale = Large;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
  addcloud(x, y - 5, scale, linesize);
  addfog(x, y - 5, scale, linesize, IconSize);
}
//#########################################################################################
void Haze(int x, int y, bool IconSize, String IconName) {
  int linesize = 3, scale = Large;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
  addsun(x, y - 5, scale * 1.4, IconSize);
  addfog(x, y - 5, scale * 1.4, linesize, IconSize);
}
//#########################################################################################
void CloudCover(int x, int y, int CCover, int text_x, int text_y) {
  addcloud(x - 9, y - 3, Small * 0.5, 2); // Cloud top left
  addcloud(x + 3, y - 3, Small * 0.5, 2); // Cloud top right
  addcloud(x, y,         Small * 0.5, 2); // Main cloud
  SetUIFont(UI_FONT_12);
  drawString(text_x, text_y, String(CCover) + "%", LEFT);
}
//#########################################################################################
void Humidity(int x, int y) {
  display.fillCircle(x, y + 2, 4, GxEPD_BLACK);
  display.fillTriangle(x - 4, y + 2, x, y - 8, x + 4, y + 2, GxEPD_BLACK);
}
//#########################################################################################
void Visibility(int x, int y, String Visi, int text_x, int text_y) {
  float start_angle = 0.52, end_angle = 2.61;
  int r = 10;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    display.drawPixel(x + r * cos(i), y - r / 2 + r * sin(i), GxEPD_BLACK);
    display.drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i), GxEPD_BLACK);
  }
  start_angle = 3.61; end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    display.drawPixel(x + r * cos(i), y + r / 2 + r * sin(i), GxEPD_BLACK);
    display.drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i), GxEPD_BLACK);
  }
  display.fillCircle(x, y, r / 4, GxEPD_BLACK);
  SetUIFont(UI_FONT_12);
  drawString(text_x, text_y, Visi, LEFT);
}
//#########################################################################################
void addmoon(int x, int y, int scale, bool IconSize) {
  if (IconSize == LargeIcon) {
    x = x + 12; y = y + 12;
    display.fillCircle(x - 50, y - 55, scale, GxEPD_BLACK);
    display.fillCircle(x - 35, y - 55, scale * 1.6, GxEPD_WHITE);
  }
  else
  {
    display.fillCircle(x - 20, y - 12, scale, GxEPD_BLACK);
    display.fillCircle(x - 15, y - 12, scale * 1.6, GxEPD_WHITE);
  }
}
//#########################################################################################
void Nodata(int x, int y, bool IconSize, String IconName) {
  if (IconSize == LargeIcon) SetUIFont(UI_FONT_24); else SetUIFont(UI_FONT_10);
  drawString(x - 3, y - 8, "?", CENTER);
  SetUIFont(UI_FONT_08);
}
//#########################################################################################
void DrawBattery(int x, int y) {
  display.drawRect(x + 15, y - 12, 19, 10, GxEPD_BLACK);
  display.fillRect(x + 34, y - 10, 2, 5, GxEPD_BLACK);
  display.fillRect(x + 17, y - 10, 15, 6, GxEPD_BLACK);
  drawString(x + 65, y - 11, "USB", RIGHT);
}
//#########################################################################################
void DrawNoonMidnightMarkers(int x_pos, int y_pos, int gwidth, int gheight) {
  if (WxForecast[0].Dt <= 0) return;

  const long start_local = long(WxForecast[0].Dt) + long(WxConditions[0].Timezone);
  const long marker_step = 12L * 3600L;
  const long chart_span = 72L * 3600L;
  long marker_time = ((start_local / marker_step) + 1) * marker_step;

  while (marker_time < start_local + chart_span) {
    const float hours_from_start = (marker_time - start_local) / 3600.0;
    const int marker_x = x_pos + int(hours_from_start * gwidth / 72.0);
    const int marker_hour = (marker_time / 3600L) % 24L;
    const int dash = marker_hour == 0 ? 3 : 1;
    const int gap = marker_hour == 0 ? 3 : 4;

    for (int marker_y = y_pos + 1; marker_y < y_pos + gheight; marker_y += dash + gap) {
      display.drawFastVLine(marker_x, marker_y, dash, GxEPD_BLACK);
    }
    marker_time += marker_step;
  }
}
//#########################################################################################
/* (C) D L BIRD
    This function will draw a graph on a ePaper/TFT/LCD display using data from an array containing data to be graphed.
    The variable 'max_readings' determines the maximum number of data elements for each array. Call it with the following parametric data:
    x_pos - the x axis top-left position of the graph
    y_pos - the y-axis top-left position of the graph, e.g. 100, 200 would draw the graph 100 pixels along and 200 pixels down from the top-left of the screen
    width - the width of the graph in pixels
    height - height of the graph in pixels
    Y1_Max - sets the scale of plotted data, for example 5000 would scale all data to a Y-axis of 5000 maximum
    data_array1 is parsed by value, externally they can be called anything else, e.g. within the routine it is called data_array1, but externally could be temperature_readings
    auto_scale - a logical value (TRUE or FALSE) that switches the Y-axis autoscale On or Off
    barchart_on - a logical value (TRUE or FALSE) that switches the drawing mode between bar and line graphs
    barchart_colour - a sets the title and graph plotting colour
    If called with Y!_Max value of 500 and the data never goes above 500, then autoscale will retain a 0-500 Y scale, if on, the scale increases/decreases to match the data.
    auto_scale_margin, e.g. if set to 1000 then autoscale increments the scale by 1000 steps.
*/
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode) {
#define auto_scale_margin 0 // Sets the autoscale increment, so axis steps up in units of e.g. 3
#define y_minor_axis 5      // 5 y-axis division markers
  float maxYscale = -10000;
  float minYscale =  10000;
  int last_x, last_y;
  float x1, y1, x2, y2;
  if (auto_scale == true) {
    for (int i = 0; i < readings; i++ ) {
      if (DataArray[i] >= maxYscale) maxYscale = DataArray[i];
      if (DataArray[i] <= minYscale) minYscale = DataArray[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale);
  }
  if (Y1Max == Y1Min) Y1Max = Y1Min + 1;
  // Draw the graph
  last_x = x_pos;
  last_y = y_pos + (Y1Max - constrain(DataArray[0], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  display.drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, GxEPD_BLACK);
  SetUIFont(UI_FONT_08);
  drawString(x_pos + gwidth / 2, y_pos - 13, title, CENTER);
  DrawNoonMidnightMarkers(x_pos, y_pos, gwidth, gheight);
  for (int hour = 1; hour < 72; hour++) {
    int tick_x = x_pos + int(hour * gwidth / 72.0);
    display.drawFastVLine(tick_x, y_pos + gheight - 2, 3, GxEPD_BLACK);
  }
  // Draw the data
  for (int gx = 0; gx < readings; gx++) {
    y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
    if (barchart_mode) {
      x2 = x_pos + gx * (gwidth / readings) + 1;
      display.fillRect(x2, y2, (gwidth / readings) - 2, y_pos + gheight - y2 + 2, GxEPD_BLACK);
    }
    else
    {
      x2 = x_pos + gx * gwidth / (readings - 1); // max_readings is the global variable that sets the maximum data that can be plotted
      display.drawLine(last_x, last_y, x2, y2, GxEPD_BLACK);
    }
    last_x = x2;
    last_y = y2;
  }
  //Draw the Y-axis scale
#define number_of_dashes 15
  int last_axis_label = 32767;
  const bool rainfall_axis = title == TXT_RAINFALL_IN || title == TXT_RAINFALL_MM;
  for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
    for (int j = 0; j < number_of_dashes; j++) { // Draw dashed graph grid lines
      if (spacing < y_minor_axis) display.drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), GxEPD_BLACK);
    }
    float axis_value = Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing;
    if (rainfall_axis) {
      drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis - 5, String(axis_value + 0.01, 1), RIGHT);
    }
    else {
      int axis_label = round(axis_value);
      if (spacing == 0 || spacing == y_minor_axis || axis_label != last_axis_label) {
        drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis - 5, String(axis_label), RIGHT);
        last_axis_label = axis_label;
      }
    }
  }
  for (int i = 0; i <= 3; i++) {
    drawString(x_pos + gwidth / 3 * i, y_pos + gheight + 3, String(i), CENTER);
  }
  String days_label = TXT_DAYS;
  days_label.replace("(", "");
  days_label.replace(")", "");
  drawString(x_pos + gwidth / 2, y_pos + gheight + 10, days_label, CENTER);
}
//#########################################################################################
void drawString(int x, int y, String text, alignment align) {
  display.setTextWrap(false);
  int16_t w = u8g2Fonts.getUTF8Width(text.c_str());
  int16_t ascent = u8g2Fonts.getFontAscent();
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  u8g2Fonts.setCursor(x, y + ascent);
  u8g2Fonts.print(text);
}
//#########################################################################################
void drawStringMaxWidth(int x, int y, unsigned int text_width, String text, alignment align) {
  int16_t w = u8g2Fonts.getUTF8Width(text.c_str());
  int16_t line_height = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent() + 2;
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  u8g2Fonts.setCursor(x, y + u8g2Fonts.getFontAscent());
  if (text.length() > text_width * 2) {
    SetUIFont(UI_FONT_10);
    line_height = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent() + 2;
    text_width = 42;
    y = y - 3;
  }
  u8g2Fonts.println(text.substring(0, text_width));
  if (text.length() > text_width) {
    u8g2Fonts.setCursor(x, y + line_height + u8g2Fonts.getFontAscent());
    String secondLine = text.substring(text_width);
    secondLine.trim(); // Remove any leading spaces
    u8g2Fonts.println(secondLine);
  }
}
//#########################################################################################
void InitialiseDisplay() {
  CrowPanelEPD::begin();
  DisplayInitialised = true;
  u8g2Fonts.begin(display); // connect u8g2 procedures to Adafruit GFX
  u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);             // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
  SetUIFont(UI_FONT_10);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  display.fillScreen(GxEPD_WHITE);
}
/*
  Version 12.0 reformatted to use u8g2 fonts
  1.  Screen layout revised
  2.  Made consitent with other versions specifically 7x5 variant
  3.  Introduced Visibility in Metres, Cloud cover in % and RH in %
  4.  Correct sunrise/sunset time when in imperial mode.

  Version 12.1 Clarified Waveshare ESP32 driver board connections

  Version 12.2 Changed GxEPD2 initialisation from 115200 to 0
  1.  display.init(115200); becomes display.init(0); to stop blank screen following update to GxEPD2

  Version 12.3
  1. Added 20-secs to allow for slow ESP32 RTC timers

  Version 12.4
  1. Improved graph drawing function for negative numbers Line 808

  Version 12.5
  1. Modified for GxEPD2 changes

  Version 12.6-crowpanel-green-tides
  1. Uses Elecrow green-sticker CrowPanel framebuffer refresh instead of GxEPD2
  2. Adds static weather/tide preview environment
*/
