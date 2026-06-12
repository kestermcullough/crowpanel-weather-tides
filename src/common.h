//

#include <Arduino.h>
#include <HTTPClient.h>

#ifndef CROWPANEL_USE_OPEN_METEO
#define CROWPANEL_USE_OPEN_METEO 0
#endif

typedef struct { // For current Day and Day 1, 2, 3, etc
  String Time;
  float  High;
  float  Low;
} HL_record_type;

HL_record_type  HLReadings[max_readings];

typedef struct { // For current Day and Day 1, 2, 3, etc
  int      Dt;
  String   Period;
  String   Icon;
  String   Trend;
  String   Main0;
  String   Forecast0;
  String   Forecast1;
  String   Forecast2;
  String   Description;
  String   Time;
  String   Country;
  float    lat;
  float    lon;
  float    Temperature;
  float    FeelsLike;
  float    Humidity;
  float    DewPoint;
  float    High;
  float    Low;
  float    Winddir;
  float    Windspeed;
  float    Rainfall;
  float    Snowfall;
  float    Pressure;
  int      Cloudcover;
  int      Visibility;
  int      Sunrise;
  int      Sunset;
  int      Moonrise;
  int      Moonset;
  int      Timezone;
  float    UVI;
  float    PoP;
} Forecast_record_type;

Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];
Forecast_record_type  Daily[8];

bool ReceiveOneCallWeather(WiFiClient& client, bool print);
#if CROWPANEL_USE_OPEN_METEO
bool DecodeOpenMeteoWeather(const String& json, bool print);
#else
bool DecodeOneCallWeather(WiFiClient& json, bool print);
#endif
void Convert_Readings_to_Imperial();
String ConvertUnixTime(int unix_time);
float mm_to_inches(float value_mm);
float hPa_to_inHg(float value_hPa);
int JulianDate(int d, int m, int y);
float SumOfPrecip(float DataArray[], int readings);
String TitleCase(String text);
double NormalizedMoonPhase(int d, int m, int y);
String OpenMeteoDescription(int weather_code);
String OpenMeteoIcon(int weather_code);
String OpenMeteoForecastUri();

//#########################################################################################
void Convert_Readings_to_Imperial() {
  WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
  WxForecast[1].Rainfall   = mm_to_inches(WxForecast[1].Rainfall);
  WxForecast[1].Snowfall   = mm_to_inches(WxForecast[1].Snowfall);
}
//#########################################################################################
String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  time_t tm = unix_time;
  struct tm *now_tm = gmtime(&tm);
  char output[40];
  if (Units == "M") {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  else {
    strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
  }
  return output;
}
//#########################################################################################
bool ReceiveOneCallWeather(WiFiClient& client, bool print) {
#if CROWPANEL_USE_OPEN_METEO
  Serial.println("Rx Open-Meteo weather data...");
  client.stop();
  HTTPClient http;
  http.begin(client, "api.open-meteo.com", 80, OpenMeteoForecastUri());
  http.useHTTP10(true);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    bool decoded = DecodeOpenMeteoWeather(response, print);
    client.stop();
    http.end();
    return decoded;
  }
  Serial.printf("Open-Meteo HTTP status: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
  String response = http.getString();
  if (response.length() > 0) {
    Serial.println(response.substring(0, 240));
  }
  client.stop();
  http.end();
  return false;
#else
  // Test call: http://api.openweathermap.org/data/3.0/onecall?lat=33&lon=-112&APPID=1a838280c1f7a40c3f8a5e5bc573e22d&mode=json&units=metric&lang=US&exclude=minutely
  Serial.println("Rx weather data...");
  const String units = (Units == "M" ? "metric" : "imperial");
  client.stop(); // close connection before sending a new request
  HTTPClient http;    
  // Update for API 3.0 June '24
  String uri = "/data/3.0/onecall?lat=" + LAT + "&lon=" + LON + "&appid=" + apikey + "&mode=json&units=" + units + "&lang=" + Language + "&exclude=minutely";
  http.begin(client, server, 80, uri);
  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK) {
    bool decoded = DecodeOneCallWeather(http.getStream(), print);
    client.stop();
    http.end();
    return decoded;
  }
  else
  {
    Serial.printf("OpenWeather HTTP status: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    String response = http.getString();
    if (response.length() > 0) {
      Serial.println(response.substring(0, 240));
    }
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
#endif
}
//#######################################################################################
#if CROWPANEL_USE_OPEN_METEO
String OpenMeteoForecastUri() {
  const String temperature_unit = (Units == "M" ? "celsius" : "fahrenheit");
  const String wind_speed_unit  = (Units == "M" ? "ms" : "mph");
  const String precipitation_unit = (Units == "M" ? "mm" : "inch");
#ifndef OPEN_METEO_TIMEZONE
#define OPEN_METEO_TIMEZONE "America%2FNew_York"
#endif
  return "/v1/forecast?latitude=" + LAT + "&longitude=" + LON +
         "&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,rain,snowfall,weather_code,cloud_cover,pressure_msl,visibility,wind_speed_10m,wind_direction_10m" +
         "&hourly=temperature_2m,relative_humidity_2m,dew_point_2m,apparent_temperature,precipitation,rain,snowfall,weather_code,cloud_cover,pressure_msl,visibility,wind_speed_10m,wind_direction_10m" +
         "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum,rain_sum,snowfall_sum,sunrise,sunset" +
         "&temperature_unit=" + temperature_unit +
         "&wind_speed_unit=" + wind_speed_unit +
         "&precipitation_unit=" + precipitation_unit +
         "&timezone=" + String(OPEN_METEO_TIMEZONE) +
         "&timeformat=unixtime&forecast_days=8&forecast_hours=72";
}
//#######################################################################################
String OpenMeteoIcon(int weather_code) {
  if (weather_code == 0) return "01d";
  if (weather_code == 1) return "02d";
  if (weather_code == 2) return "03d";
  if (weather_code == 3 || weather_code == 45 || weather_code == 48) return "04d";
  if ((weather_code >= 51 && weather_code <= 67) || (weather_code >= 80 && weather_code <= 82)) return "10d";
  if (weather_code >= 71 && weather_code <= 77) return "13d";
  if (weather_code >= 85 && weather_code <= 86) return "13d";
  if (weather_code >= 95) return "11d";
  return "03d";
}
//#######################################################################################
String OpenMeteoDescription(int weather_code) {
  if (weather_code == 0) return "Clear sky";
  if (weather_code == 1) return "Mainly clear";
  if (weather_code == 2) return "Partly cloudy";
  if (weather_code == 3) return "Overcast";
  if (weather_code == 45 || weather_code == 48) return "Fog";
  if (weather_code >= 51 && weather_code <= 57) return "Drizzle";
  if (weather_code >= 61 && weather_code <= 67) return "Rain";
  if (weather_code >= 71 && weather_code <= 77) return "Snow";
  if (weather_code >= 80 && weather_code <= 82) return "Rain showers";
  if (weather_code >= 85 && weather_code <= 86) return "Snow showers";
  if (weather_code >= 95) return "Thunderstorm";
  return "Forecast";
}
//#######################################################################################
bool DecodeOpenMeteoWeather(const String& json, bool print) {
  if (print) Serial.println("Decoding Open-Meteo data...");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    Serial.println(json.substring(0, 120));
    return false;
  }

  WxConditions[0].Timezone = doc["utc_offset_seconds"] | gmtOffset_sec;
  JsonObject current = doc["current"];
  int current_code = current["weather_code"] | 0;
  WxConditions[0].Description = OpenMeteoDescription(current_code);
  WxConditions[0].Icon        = OpenMeteoIcon(current_code);
  WxConditions[0].Temperature = current["temperature_2m"] | 0.0;
  WxConditions[0].FeelsLike   = current["apparent_temperature"] | WxConditions[0].Temperature;
  WxConditions[0].Pressure    = current["pressure_msl"] | 0.0;
  WxConditions[0].Humidity    = current["relative_humidity_2m"] | 0.0;
  WxConditions[0].DewPoint    = 0;
  WxConditions[0].Cloudcover  = current["cloud_cover"] | 0;
  WxConditions[0].Visibility  = current["visibility"] | 0;
  WxConditions[0].Windspeed   = current["wind_speed_10m"] | 0.0;
  WxConditions[0].Winddir     = current["wind_direction_10m"] | 0.0;
  WxConditions[0].Rainfall    = current["rain"] | current["precipitation"] | 0.0;
  WxConditions[0].Snowfall    = current["snowfall"] | 0.0;

  JsonObject hourly = doc["hourly"];
  JsonArray hourly_time = hourly["time"];
  JsonArray hourly_temp = hourly["temperature_2m"];
  JsonArray hourly_feels = hourly["apparent_temperature"];
  JsonArray hourly_humidity = hourly["relative_humidity_2m"];
  JsonArray hourly_dew = hourly["dew_point_2m"];
  JsonArray hourly_pressure = hourly["pressure_msl"];
  JsonArray hourly_visibility = hourly["visibility"];
  JsonArray hourly_wind = hourly["wind_speed_10m"];
  JsonArray hourly_wind_dir = hourly["wind_direction_10m"];
  JsonArray hourly_rain = hourly["rain"];
  JsonArray hourly_precip = hourly["precipitation"];
  JsonArray hourly_snow = hourly["snowfall"];
  JsonArray hourly_code = hourly["weather_code"];

  const int hourly_count = min(max_readings, int(hourly_time.size()));
  for (int r = 0; r < hourly_count; r++) {
    int weather_code = hourly_code[r] | 0;
    WxForecast[r].Dt          = hourly_time[r] | 0;
    WxForecast[r].Temperature = hourly_temp[r] | 0.0;
    WxForecast[r].FeelsLike   = hourly_feels[r] | WxForecast[r].Temperature;
    WxForecast[r].Humidity    = hourly_humidity[r] | 0.0;
    WxForecast[r].DewPoint    = hourly_dew[r] | 0.0;
    WxForecast[r].Pressure    = hourly_pressure[r] | 0.0;
    WxForecast[r].Visibility  = hourly_visibility[r] | 0;
    WxForecast[r].Windspeed   = hourly_wind[r] | 0.0;
    WxForecast[r].Winddir     = hourly_wind_dir[r] | 0.0;
    WxForecast[r].Rainfall    = hourly_rain[r] | hourly_precip[r] | 0.0;
    WxForecast[r].Snowfall    = hourly_snow[r] | 0.0;
    WxForecast[r].Cloudcover  = hourly["cloud_cover"][r] | 0;
    WxForecast[r].Icon        = OpenMeteoIcon(weather_code);
    WxForecast[r].Description = OpenMeteoDescription(weather_code);
  }
  if (WxConditions[0].Visibility <= 0 && hourly_count > 0) {
    WxConditions[0].Visibility = WxForecast[0].Visibility;
  }
  if (WxConditions[0].Cloudcover <= 0 && hourly_count > 0) {
    WxConditions[0].Cloudcover = WxForecast[0].Cloudcover;
  }

  JsonObject daily = doc["daily"];
  JsonArray daily_time = daily["time"];
  JsonArray daily_code = daily["weather_code"];
  JsonArray daily_high = daily["temperature_2m_max"];
  JsonArray daily_low = daily["temperature_2m_min"];
  JsonArray daily_rain = daily["rain_sum"];
  JsonArray daily_precip = daily["precipitation_sum"];
  JsonArray daily_snow = daily["snowfall_sum"];
  JsonArray daily_sunrise = daily["sunrise"];
  JsonArray daily_sunset = daily["sunset"];
  const int daily_count = min(8, int(daily_time.size()));
  for (int r = 0; r < daily_count; r++) {
    int weather_code = daily_code[r] | 0;
    Daily[r].Dt          = daily_time[r] | 0;
    Daily[r].Description = OpenMeteoDescription(weather_code);
    Daily[r].Temperature = daily_high[r] | 0.0;
    Daily[r].High        = daily_high[r] | 0.0;
    Daily[r].Low         = daily_low[r] | 0.0;
    Daily[r].Rainfall    = daily_rain[r] | daily_precip[r] | 0.0;
    Daily[r].Snowfall    = daily_snow[r] | 0.0;
    Daily[r].Icon        = OpenMeteoIcon(weather_code);
  }
  if (daily_count > 0) {
    WxConditions[0].High    = Daily[0].High;
    WxConditions[0].Low     = Daily[0].Low;
    WxConditions[0].Sunrise = daily_sunrise[0] | 0;
    WxConditions[0].Sunset  = daily_sunset[0] | 0;
  }

  float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure;
  pressure_trend = ((int)(pressure_trend * 10)) / 10.0;
  WxConditions[0].Trend = "0";
  if (pressure_trend > 0) WxConditions[0].Trend = "+";
  if (pressure_trend < 0) WxConditions[0].Trend = "-";

  if (Units == "I") {
    WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
    for (int r = 0; r < hourly_count; r++) WxForecast[r].Pressure = hPa_to_inHg(WxForecast[r].Pressure);
  }

  if (print) {
    Serial.println("Fore: " + WxConditions[0].Description);
    Serial.println("Temp: " + String(WxConditions[0].Temperature));
    Serial.println("WSpd: " + String(WxConditions[0].Windspeed));
    Serial.println("Visi: " + String(WxConditions[0].Visibility));
    Serial.println("CCov: " + String(WxConditions[0].Cloudcover));
  }
  return true;
}
//#######################################################################################
#else
bool DecodeOneCallWeather(WiFiClient& json, bool print) {
  if (print) Serial.println("Decoding Wx Data...");
  JsonDocument doc;                                        // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {                                             // Test if parsing succeeds.
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  const char* TempString;
  Serial.println("\nDecoding data...");
  if (print) Serial.println("Displaying CURRENT conditions..."); // Needed for the main display items
  WxConditions[0].Timezone    = doc["timezone_offset"];      if (print) Serial.println("TZon: " + String(WxConditions[0].Timezone));
  JsonObject current = doc["current"];
  JsonObject current_weather_0 = current["weather"][0];
  int weather_id = current_weather_0["id"]; // 800
  const char* main_weather = current_weather_0["main"]; // "Clear"
  const char* weather = current_weather_0["description"]; // "Clear Skies"
  WxConditions[0].Description = String(weather);              if (print) Serial.println("Fore: " + String(weather));
  const char* current_icon = current_weather_0["icon"];
  WxConditions[0].Icon        = current_icon;                 if (print) Serial.println("Icon: " + String(WxConditions[0].Icon));
  int sunriseL =  int(WxConditions[0].Timezone) + int(current["sunrise"]);
  WxConditions[0].Sunrise     = current["sunrise"];           if (print) Serial.println("SRis: " + String(WxConditions[0].Sunrise) + " " + ConvertUnixTime(sunriseL));
  int sunsetL  =  int(WxConditions[0].Timezone) + int(current["sunset"]);
  WxConditions[0].Sunset      = current["sunset"];            if (print) Serial.println("SSet: " + String(WxConditions[0].Sunset)  + " " + ConvertUnixTime(sunsetL));
  WxConditions[0].Temperature = current["temp"];              if (print) Serial.println("Temp: " + String(WxConditions[0].Temperature));
  WxConditions[0].FeelsLike   = current["feels_like"];        if (print) Serial.println("FLik: " + String(WxConditions[0].FeelsLike));
  WxConditions[0].Pressure    = current["pressure"];          if (print) Serial.println("Pres: " + String(WxConditions[0].Pressure));
  WxConditions[0].Humidity    = current["humidity"];          if (print) Serial.println("Humi: " + String(WxConditions[0].Humidity));
  WxConditions[0].DewPoint    = current["dew_point"];         if (print) Serial.println("DewP: " + String(WxConditions[0].DewPoint));
  WxConditions[0].UVI         = current["uvi"];               if (print) Serial.println("UVin: " + String(WxConditions[0].UVI));
  WxConditions[0].Cloudcover  = current["clouds"];            if (print) Serial.println("CCov: " + String(WxConditions[0].Cloudcover));
  WxConditions[0].Visibility  = current["visibility"];        if (print) Serial.println("Visi: " + String(WxConditions[0].Visibility));
  WxConditions[0].Windspeed   = current["wind_speed"];        if (print) Serial.println("WSpd: " + String(WxConditions[0].Windspeed));
  WxConditions[0].Winddir     = current["wind_deg"];          if (print) Serial.println("WDir: " + String(WxConditions[0].Winddir));

  JsonArray hourlyArray = doc["hourly"];

  if (print) Serial.println("\nDisplaying 48-hrs of HOURLY data..."); // Needed for the graphs
  for (int r = 0; r < max_readings; r++) {
    JsonObject hourly = hourlyArray[r];
    if (print) Serial.println("Day (Hour)-" + String(r) + " --------------");
    WxForecast[r].Dt          = hourly["dt"];                 if (print) Serial.println(ConvertUnixTime(WxForecast[r].Dt));
    WxForecast[r].Temperature = hourly["temp"];               if (print) Serial.println("Temp: " + String(WxForecast[r].Temperature));
    WxForecast[r].FeelsLike   = hourly["feels_like"];         if (print) Serial.println("FLik: " + String(WxForecast[r].FeelsLike));
    WxForecast[r].Pressure    = hourly["pressure"];           if (print) Serial.println("Pres: " + String(WxForecast[r].Pressure));
    WxForecast[r].Humidity    = hourly["humidity"];           if (print) Serial.println("Humi: " + String(WxForecast[r].Humidity));
    WxForecast[r].DewPoint    = hourly["dew_point"];          if (print) Serial.println("DewP: " + String(WxForecast[r].DewPoint));
    WxForecast[r].Windspeed   = hourly["wind_speed"];         if (print) Serial.println("WSpd: " + String(WxForecast[r].Windspeed));
    WxForecast[r].Winddir     = hourly["wind_deg"];           if (print) Serial.println("WDir: " + String(WxForecast[r].Winddir));
    WxForecast[r].Rainfall    = hourly["rain"]["1h"];         if (print) Serial.println("Rain: " + String(WxForecast[r].Rainfall));
    WxForecast[r].Snowfall    = hourly["snow"]["1h"];         if (print) Serial.println("Snow: " + String(WxForecast[r].Snowfall));
    WxForecast[r].Rainfall    = hourly["rain"]["1h"];         if (print) Serial.println("Rain: " + String(WxForecast[r].Rainfall));
    WxForecast[r].Snowfall    = hourly["snow"]["1h"];         if (print) Serial.println("Snow: " + String(WxForecast[r].Snowfall));
    JsonObject hourly_weather = hourly["weather"][0];
    WxForecast[r].Icon        = hourly["weather"][0]["icon"].as<const char*>(); if (print) Serial.println("Icon: " + String(WxForecast[r].Icon));
  }

  JsonArray daily = doc["daily"];
  if (print) Serial.println("\nDisplaying DAILY Data --------------"); // Neded for the 7-day forecast section
  for (int r = 0; r < 8; r++) { // Maximum of 8-days!
    if (print) Serial.println("\nData for DAY - " + String(r) + " --------------");
    JsonObject daily_values = daily[r];
    Daily[r].Dt          = daily_values["dt"];                                   if (print) Serial.println(ConvertUnixTime(Daily[r].Dt));
    Daily[r].Description = daily_values["summary"].as<const char*>();            if (print) Serial.println("Summary: " + Daily[r].Description);
    Daily[r].Temperature = daily_values["temp"]["day"];                          if (print) Serial.println("Temp   : " + String(Daily[r].Temperature));
    Daily[r].High        = daily_values["temp"]["max"];                          if (print) Serial.println("High   : " + String(Daily[r].High));
    Daily[r].Low         = daily_values["temp"]["min"];                          if (print) Serial.println("Low    : " + String(Daily[r].Low));
    Daily[r].Humidity    = daily_values["humidity"];                             if (print) Serial.println("Humi   : " + String(Daily[r].Humidity));
    Daily[r].PoP         = daily_values["pop"];                                  if (print) Serial.println("PoP    : " + String(Daily[r].PoP*100, 0) + "%");
    Daily[r].UVI         = daily_values["uvi"];                                  if (print) Serial.println("UVI    : " + String(Daily[r].UVI, 1));
    Daily[r].Rainfall    = daily_values["rain"];                                 if (print) Serial.println("Rain   : " + String(Daily[r].Rainfall));
    Daily[r].Snowfall    = daily_values["snow"];                                 if (print) Serial.println("Snow   : " + String(Daily[r].Snowfall));
    Daily[r].Icon        = daily_values["weather"][0]["icon"].as<const char*>(); if (print) Serial.println("Icon   : " + String(Daily[r].Icon));
  }
  //------------------------------------------
  float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure; // Measure pressure slope between ~now and later
  pressure_trend = ((int)(pressure_trend * 10)) / 10.0; // Remove any small variations of less than 0.1
  WxConditions[0].Trend = "=";
  if (pressure_trend > 0)  WxConditions[0].Trend = "+";
  if (pressure_trend < 0)  WxConditions[0].Trend = "-";
  if (pressure_trend == 0) WxConditions[0].Trend = "0";
  if (Units == "I") Convert_Readings_to_Imperial();
  return true;
}
#endif

float mm_to_inches(float value_mm){
  return 0.0393701 * value_mm;
}

float hPa_to_inHg(float value_hPa){
  return 0.02953 * value_hPa;
}

int JulianDate(int d, int m, int y) {
  int mm, yy, k1, k2, k3, j;
  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12) mm = mm - 12;
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59 + 1;
  if (j > 2299160) j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time) For Gregorian calendar:
  return j;
}

float SumOfPrecip(float DataArray[], int readings) {
  float sum = 0;
  for (int i = 0; i < readings; i++) {
    sum += DataArray[i];
  }
  return sum;
}

String TitleCase(String text){
  if (text.length() > 0) {
    String temp_text = text.substring(0,1);
    temp_text.toUpperCase();
    return temp_text + text.substring(1); // Title-case the string
  }
  else return text;
}

double NormalizedMoonPhase(int d, int m, int y) {
  int j = JulianDate(d, m, y);
  //Calculate the approximate phase of the moon
  double Phase = (j + 4.867) / 29.53059;
  return (Phase - (int) Phase);
}


