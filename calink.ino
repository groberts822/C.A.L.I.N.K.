/**
 * CALINK — E-Ink Wall Calendar
 * ESP32 + Waveshare 7.5" B/W e-Paper + Google Calendar via Google Apps Script
 *
 * Features:
 *  - Google Calendar sync via Google Apps Script webhook
 *  - OpenWeatherMap weather + daily quote
 *  - Week / Day / Agenda view (cycle on each wake)
 *  - Deep sleep between refreshes (saves battery)
 *  - NTP time sync
 *
 * Libraries needed (install via Arduino Library Manager):
 *  - GxEPD2 by ZinggJM
 *  - ArduinoJson by Benoit Blanchon
 *  - HTTPClient (built into ESP32 Arduino core)
 *  - WiFi (built into ESP32 Arduino core)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <time.h>

// ─── USER CONFIG ────────────────────────────────────────────────────────────
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"

// Google Apps Script Web App URL (see README for setup)
#define GAS_URL           "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec"

// OpenWeatherMap API (free tier)
#define OWM_API_KEY       "YOUR_OWM_API_KEY"
#define OWM_CITY          "New York"
#define OWM_UNITS         "imperial"   // "metric" for Celsius

// Timezone (NTP)
#define NTP_SERVER        "pool.ntp.org"
#define GMT_OFFSET_SEC    -18000       // EST = -5h. Change for your timezone.
#define DAYLIGHT_OFFSET   3600

// Refresh interval — how often to wake and refresh (in seconds)
// 3600 = every hour. Increase to save battery.
#define SLEEP_SECONDS     3600

// View cycling — stored in RTC memory, persists through deep sleep
// 0 = Week view, 1 = Day view, 2 = Agenda view
RTC_DATA_ATTR int currentView = 0;
RTC_DATA_ATTR int bootCount = 0;
// ────────────────────────────────────────────────────────────────────────────

// Waveshare 7.5" B/W display — pins for Waveshare ESP32 Driver Board
#define EPD_CS    15
#define EPD_DC    27
#define EPD_RST   26
#define EPD_BUSY  25

GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(
  GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ─── DATA STRUCTS ───────────────────────────────────────────────────────────
struct CalEvent {
  String title;
  String startTime;
  String endTime;
  String date;
  bool allDay;
};

struct WeatherData {
  String description;
  float tempNow;
  float tempHigh;
  float tempLow;
  int humidity;
  String icon;
};

#define MAX_EVENTS 20
CalEvent events[MAX_EVENTS];
int eventCount = 0;
WeatherData weather;
String dailyQuote = "";
String currentDateStr = "";
struct tm timeInfo;

// ─── SETUP ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  bootCount++;
  Serial.printf("Boot #%d | View: %d\n", bootCount, currentView);

  display.init(115200, true, 50, false);
  display.setRotation(0);

  connectWifi();
  syncTime();
  fetchCalendar();
  fetchWeather();
  fetchQuote();
  renderDisplay();

  // Cycle to next view for next wake
  currentView = (currentView + 1) % 3;

  Serial.printf("Going to sleep for %d seconds\n", SLEEP_SECONDS);
  display.hibernate();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Never reached — deep sleep restarts setup()
}

// ─── WIFI ───────────────────────────────────────────────────────────────────
void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connected!");
  } else {
    Serial.println(" FAILED. Rendering from cache or blank.");
  }
}

// ─── NTP TIME ───────────────────────────────────────────────────────────────
void syncTime() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to get NTP time");
    return;
  }
  char buf[32];
  strftime(buf, sizeof(buf), "%A, %B %d %Y", &timeInfo);
  currentDateStr = String(buf);
  Serial.println("Time: " + currentDateStr);
}

// ─── GOOGLE CALENDAR (via Google Apps Script) ───────────────────────────────
void fetchCalendar() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(GAS_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<8192> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      Serial.println("JSON parse error: " + String(err.c_str()));
      return;
    }
    JsonArray arr = doc.as<JsonArray>();
    eventCount = 0;
    for (JsonObject ev : arr) {
      if (eventCount >= MAX_EVENTS) break;
      events[eventCount].title     = ev["title"].as<String>();
      events[eventCount].startTime = ev["start"].as<String>();
      events[eventCount].endTime   = ev["end"].as<String>();
      events[eventCount].date      = ev["date"].as<String>();
      events[eventCount].allDay    = ev["allDay"].as<bool>();
      eventCount++;
    }
    Serial.printf("Fetched %d events\n", eventCount);
  } else {
    Serial.printf("Calendar fetch failed: HTTP %d\n", code);
  }
  http.end();
}

// ─── WEATHER ────────────────────────────────────────────────────────────────
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
               String(OWM_CITY) + "&appid=" + String(OWM_API_KEY) +
               "&units=" + String(OWM_UNITS);

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<2048> doc;
    deserializeJson(doc, payload);
    weather.description = doc["weather"][0]["description"].as<String>();
    weather.tempNow     = doc["main"]["temp"].as<float>();
    weather.tempHigh    = doc["main"]["temp_max"].as<float>();
    weather.tempLow     = doc["main"]["temp_min"].as<float>();
    weather.humidity    = doc["main"]["humidity"].as<int>();
    Serial.printf("Weather: %.1f°, %s\n", weather.tempNow, weather.description.c_str());
  } else {
    Serial.printf("Weather fetch failed: HTTP %d\n", code);
  }
  http.end();
}

// ─── DAILY QUOTE (quotable.io free API) ─────────────────────────────────────
void fetchQuote() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("https://api.quotable.io/random?maxLength=100");
  int code = http.GET();
  if (code == 200) {
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, http.getString());
    dailyQuote = "\"" + doc["content"].as<String>() + "\" — " + doc["author"].as<String>();
    Serial.println("Quote: " + dailyQuote);
  }
  http.end();
}

// ─── DISPLAY RENDERING ──────────────────────────────────────────────────────
void renderDisplay() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    drawHeader();
    drawWeather();

    switch (currentView) {
      case 0: drawWeekView();   break;
      case 1: drawDayView();    break;
      case 2: drawAgendaView(); break;
    }

    drawFooter();
  } while (display.nextPage());
}

// ─── HEADER (date + view label) ─────────────────────────────────────────────
void drawHeader() {
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(10, 35);
  display.print(currentDateStr);

  // View label top right
  display.setFont(&FreeMonoBold9pt7b);
  String viewLabel[] = {"WEEK", "DAY", "AGENDA"};
  display.setCursor(680, 20);
  display.print(viewLabel[currentView]);

  // Divider line
  display.drawLine(0, 45, 800, 45, GxEPD_BLACK);
}

// ─── WEATHER BAR ────────────────────────────────────────────────────────────
void drawWeather() {
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 65);
  display.printf("%.0f° %s  H:%.0f° L:%.0f°  Humidity: %d%%",
    weather.tempNow, weather.description.c_str(),
    weather.tempHigh, weather.tempLow, weather.humidity);
  display.drawLine(0, 72, 800, 72, GxEPD_BLACK);
}

// ─── WEEK VIEW ──────────────────────────────────────────────────────────────
void drawWeekView() {
  display.setFont(&FreeMonoBold9pt7b);
  const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  int colW = 800 / 7;
  int y = 80;

  // Day headers
  for (int i = 0; i < 7; i++) {
    int x = i * colW;
    display.setCursor(x + 5, y + 15);
    display.print(days[i]);

    // Highlight today
    if (i == timeInfo.tm_wday) {
      display.drawRect(x, y, colW, 20, GxEPD_BLACK);
    }
  }

  // Draw events under their day column
  int evY[7] = {y + 25, y + 25, y + 25, y + 25, y + 25, y + 25, y + 25};
  for (int i = 0; i < eventCount; i++) {
    // Parse day of week from event date (simple approach)
    // event.date format: "YYYY-MM-DD"
    // Match to current week — basic implementation
    int eventDow = getDayOfWeek(events[i].date);
    if (eventDow < 0) continue;
    int x = eventDow * colW + 2;
    if (evY[eventDow] < 460) {
      display.setCursor(x, evY[eventDow]);
      // Truncate title to fit column
      String title = events[i].title;
      if (title.length() > 10) title = title.substring(0, 9) + ".";
      display.print(title);
      if (!events[i].allDay) {
        evY[eventDow] += 12;
        display.setCursor(x, evY[eventDow]);
        display.print(events[i].startTime.substring(11, 16));
      }
      evY[eventDow] += 14;
    }
  }

  // Vertical grid lines
  for (int i = 1; i < 7; i++) {
    display.drawLine(i * colW, y, i * colW, 460, GxEPD_BLACK);
  }
}

// ─── DAY VIEW ───────────────────────────────────────────────────────────────
void drawDayView() {
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(10, 100);
  display.print("Today's Events");
  display.drawLine(0, 108, 800, 108, GxEPD_BLACK);

  display.setFont(&FreeMonoBold9pt7b);
  int y = 125;
  int todayCount = 0;

  char todayStr[11];
  strftime(todayStr, sizeof(todayStr), "%Y-%m-%d", &timeInfo);

  for (int i = 0; i < eventCount; i++) {
    if (events[i].date == String(todayStr)) {
      display.setCursor(10, y);
      String timeStr = events[i].allDay ? "All Day" :
                       events[i].startTime.substring(11, 16) + " - " +
                       events[i].endTime.substring(11, 16);
      display.printf("• %s  |  %s", timeStr.c_str(), events[i].title.c_str());
      y += 22;
      todayCount++;
    }
  }

  if (todayCount == 0) {
    display.setCursor(10, y);
    display.print("No events today. Enjoy your day!");
  }
}

// ─── AGENDA VIEW ────────────────────────────────────────────────────────────
void drawAgendaView() {
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(10, 100);
  display.print("Upcoming");
  display.drawLine(0, 108, 800, 108, GxEPD_BLACK);

  display.setFont(&FreeMonoBold9pt7b);
  int y = 125;
  String lastDate = "";

  for (int i = 0; i < eventCount && y < 440; i++) {
    if (events[i].date != lastDate) {
      lastDate = events[i].date;
      y += 4;
      display.setCursor(10, y);
      display.setFont(&FreeSansBold12pt7b);
      display.print(events[i].date);
      y += 18;
      display.setFont(&FreeMonoBold9pt7b);
    }
    display.setCursor(20, y);
    String timeStr = events[i].allDay ? "All Day" :
                     events[i].startTime.substring(11, 16);
    display.printf("  %s — %s", timeStr.c_str(), events[i].title.c_str());
    y += 16;
  }
}

// ─── FOOTER (quote) ─────────────────────────────────────────────────────────
void drawFooter() {
  display.drawLine(0, 455, 800, 455, GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 472);
  // Truncate quote to fit
  String q = dailyQuote;
  if (q.length() > 110) q = q.substring(0, 107) + "...";
  display.print(q);
}

// ─── HELPERS ────────────────────────────────────────────────────────────────
// Returns 0-6 (Sun-Sat) for a date string "YYYY-MM-DD", or -1 if not this week
int getDayOfWeek(String dateStr) {
  if (dateStr.length() < 10) return -1;
  int year  = dateStr.substring(0, 4).toInt();
  int month = dateStr.substring(5, 7).toInt();
  int day   = dateStr.substring(8, 10).toInt();

  struct tm t = {0};
  t.tm_year = year - 1900;
  t.tm_mon  = month - 1;
  t.tm_mday = day;
  mktime(&t);

  // Check if within current week
  int diff = t.tm_yday - timeInfo.tm_yday;
  if (diff < -timeInfo.tm_wday || diff > (6 - timeInfo.tm_wday)) return -1;
  return t.tm_wday;
}
