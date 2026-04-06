#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <time.h>

// pin defs
#define EPD_CS    15
#define EPD_DC    27
#define EPD_RST   26
#define EPD_BUSY  25
#define BATT_PIN  34   // goes to midpoint of voltage divider (100k + 100k)

#define NTP_SERVER      "pool.ntp.org"
#define SLEEP_SECONDS   3600
#define BOOT_SCREEN_MS  3000
#define AP_NAME         "CALINK-setup"

// voltage divider math - two 100k resistors, so midpoint = Vbat/2
// ADC is 12-bit (0-4095) at 3.3V reference
// fully charged LiPo ~4.2V -> 2.1V at midpoint -> ~2613 raw
// "dead" at ~3.3V -> 1.65V -> ~2048 raw
// adding a fudge factor because the ESP32 ADC is kinda trash at the top end
#define BATT_MAX_RAW  2550
#define BATT_MIN_RAW  2048

Preferences prefs;
WebServer server(80);

// config loaded from flash
String cfg_ssid     = "";
String cfg_password = "";
String cfg_gasUrl   = "";
String cfg_owmKey   = "";
String cfg_city     = "New York";
String cfg_units    = "imperial";
int    cfg_gmtOffset = -18000;
bool   cfg_focusMode = true;

RTC_DATA_ATTR int currentView = 0;
RTC_DATA_ATTR int bootCount   = 0;

GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(
  GxEPD2_750_T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

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
};

#define MAX_EVENTS 20
CalEvent events[MAX_EVENTS];
int eventCount = 0;
WeatherData weather;
String dailyQuote    = "";
String currentDateStr = "";
struct tm timeInfo;
int batteryPct = -1;  // -1 = not read yet


// ─── setup ────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  bootCount++;

  display.init(115200, true, 50, false);
  display.setRotation(0);

  loadConfig();

  // if no wifi saved, spin up the captive portal and block until configured
  if (cfg_ssid.length() == 0) {
    runCaptivePortal();
    // after portal saves config we restart so everything comes up clean
    ESP.restart();
  }

  readBattery();

  connectWifi();
  syncTime();
  fetchCalendar();
  fetchWeather();
  fetchQuote();

  if (bootCount == 1) {
    drawBootScreen();
    delay(BOOT_SCREEN_MS);
  }

  renderDisplay();
  currentView = (currentView + 1) % 3;

  display.hibernate();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}


// ─── config / preferences ─────────────────────────────────────────────────────

void loadConfig() {
  prefs.begin("calink", true);  // read-only
  cfg_ssid      = prefs.getString("ssid",      "");
  cfg_password  = prefs.getString("pass",      "");
  cfg_gasUrl    = prefs.getString("gasUrl",    "");
  cfg_owmKey    = prefs.getString("owmKey",    "");
  cfg_city      = prefs.getString("city",      "New York");
  cfg_units     = prefs.getString("units",     "imperial");
  cfg_gmtOffset = prefs.getInt("gmtOff",       -18000);
  cfg_focusMode = prefs.getBool("focusMode",   true);
  prefs.end();
}

void saveConfig(String ssid, String pass, String gasUrl, String owmKey,
                String city, String units, int gmtOff) {
  prefs.begin("calink", false);
  prefs.putString("ssid",      ssid);
  prefs.putString("pass",      pass);
  prefs.putString("gasUrl",    gasUrl);
  prefs.putString("owmKey",    owmKey);
  prefs.putString("city",      city);
  prefs.putString("units",     units);
  prefs.putInt("gmtOff",       gmtOff);
  prefs.putBool("focusMode",   true);
  prefs.end();
}


// ─── captive portal ───────────────────────────────────────────────────────────

// serves a config page over its own AP so you can set everything from your phone
// without touching the firmware. connect to "CALINK-setup", open any http page,
// and it redirects you to the form.

void runCaptivePortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME);
  delay(500);

  // captive portal redirect - any DNS request goes to us
  // iOS and Android both check for connectivity and hit a known URL,
  // which we intercept and redirect to /setup
  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/setup");
    server.send(302, "text/plain", "");
  });

  server.on("/setup", HTTP_GET, serveSetupPage);
  server.on("/save",  HTTP_POST, handleSave);
  server.begin();

  // show a "waiting for config" screen
  drawPortalScreen();

  Serial.println("Portal up at 192.168.4.1");
  // just spin until /save posts and we restart
  while (true) {
    server.handleClient();
    delay(2);
  }
}

void serveSetupPage() {
  // inline HTML - nothing fancy, just needs to work on a phone
  String html = R"rawhtml(
<!DOCTYPE html><html><head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>CALINK Setup</title>
<style>
  body{font-family:sans-serif;max-width:480px;margin:40px auto;padding:0 16px;background:#f5f5f5}
  h2{margin-bottom:4px}
  p.sub{color:#666;font-size:13px;margin-top:0}
  label{display:block;margin-top:16px;font-size:14px;font-weight:600}
  input,select{width:100%;padding:10px;margin-top:4px;border:1px solid #ccc;border-radius:6px;
    font-size:15px;box-sizing:border-box;background:#fff}
  input[type=submit]{background:#222;color:#fff;border:none;cursor:pointer;margin-top:24px;font-size:16px}
  input[type=submit]:active{background:#444}
  .hint{font-size:12px;color:#888;margin-top:3px}
</style></head><body>
<h2>CALINK Setup</h2>
<p class='sub'>Fill this out once and it saves to the device.</p>
<form action='/save' method='POST'>
  <label>WiFi Network</label>
  <input name='ssid' placeholder='Your WiFi name' required>
  <label>WiFi Password</label>
  <input name='pass' type='password' placeholder='Your WiFi password'>
  <label>Google Apps Script URL</label>
  <input name='gasUrl' placeholder='https://script.google.com/macros/s/...' required>
  <p class='hint'>Deploy your calendar script as a web app and paste the URL here.</p>
  <label>OpenWeatherMap API Key</label>
  <input name='owmKey' placeholder='Your OWM key' required>
  <label>City</label>
  <input name='city' placeholder='New York' value='New York'>
  <label>Units</label>
  <select name='units'>
    <option value='imperial'>Imperial (°F)</option>
    <option value='metric'>Metric (°C)</option>
  </select>
  <label>UTC Offset (seconds)</label>
  <input name='gmtOff' type='number' value='-18000' placeholder='-18000 for EST'>
  <p class='hint'>EST = -18000, PST = -28800, UTC = 0, CET = 3600</p>
  <input type='submit' value='Save and connect'>
</form></body></html>
)rawhtml";

  server.send(200, "text/html", html);
}

void handleSave() {
  String ssid   = server.arg("ssid");
  String pass   = server.arg("pass");
  String gasUrl = server.arg("gasUrl");
  String owmKey = server.arg("owmKey");
  String city   = server.arg("city");
  String units  = server.arg("units");
  int gmtOff    = server.arg("gmtOff").toInt();

  if (ssid.length() == 0 || gasUrl.length() == 0 || owmKey.length() == 0) {
    server.send(400, "text/plain", "Missing required fields");
    return;
  }

  saveConfig(ssid, pass, gasUrl, owmKey, city, units, gmtOff);

  server.send(200, "text/html",
    "<html><body style='font-family:sans-serif;text-align:center;padding-top:80px'>"
    "<h2>Saved!</h2><p>CALINK is restarting and connecting to your network.<br>"
    "This page will stop working in a moment — that's normal.</p></body></html>");

  delay(1500);
  ESP.restart();
}


// ─── battery ──────────────────────────────────────────────────────────────────

void readBattery() {
  // take a few samples and average - ADC on ESP32 is noisy
  long total = 0;
  for (int i = 0; i < 8; i++) {
    total += analogRead(BATT_PIN);
    delay(5);
  }
  int raw = total / 8;

  // map to 0-100, clamp the edges
  batteryPct = map(raw, BATT_MIN_RAW, BATT_MAX_RAW, 0, 100);
  batteryPct = constrain(batteryPct, 0, 100);

  Serial.printf("Battery raw: %d -> %d%%\n", raw, batteryPct);
}


// ─── wifi / time ──────────────────────────────────────────────────────────────

void connectWifi() {
  WiFi.begin(cfg_ssid.c_str(), cfg_password.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
}

void syncTime() {
  configTime(cfg_gmtOffset, 3600, NTP_SERVER);
  if (!getLocalTime(&timeInfo)) return;
  char buf[32];
  strftime(buf, sizeof(buf), "%A, %B %d %Y", &timeInfo);
  currentDateStr = String(buf);
}


// ─── data fetching ────────────────────────────────────────────────────────────

void fetchCalendar() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(cfg_gasUrl);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<8192> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) return;
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
  }
  http.end();
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
               cfg_city + "&appid=" + cfg_owmKey + "&units=" + cfg_units;
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
  }
  http.end();
}

void fetchQuote() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin("https://api.quotable.io/random?maxLength=100");
  int code = http.GET();
  if (code == 200) {
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, http.getString());
    dailyQuote = "\"" + doc["content"].as<String>() + "\" — " + doc["author"].as<String>();
  }
  http.end();
}


// ─── display drawing ──────────────────────────────────────────────────────────

void drawPortalScreen() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(10, 10, 780, 460, GxEPD_BLACK);
    display.setFont(&FreeSansBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(220, 180);
    display.print("CALINK Setup");
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(180, 230);
    display.print("Connect to WiFi network: " AP_NAME);
    display.setCursor(180, 255);
    display.print("Then open any webpage on your phone.");
    display.setCursor(180, 310);
    display.print("Waiting for configuration...");
  } while (display.nextPage());
}

void drawBootScreen() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawRect(10, 10, 780, 460, GxEPD_BLACK);
    display.drawRect(14, 14, 772, 452, GxEPD_BLACK);
    display.setFont(&FreeSansBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(280, 200);
    display.print("CALINK");
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(220, 240);
    display.print("Calendar Ambient Low-power Ink");
    display.setCursor(245, 270);
    display.print("Always on. Always calm.");
    display.setCursor(350, 440);
    display.print("v1.1");
  } while (display.nextPage());
}

// draws the battery icon + percentage in the top-right corner of the header
// warning icon replaces the bar when under 15%
void drawBatteryIndicator() {
  if (batteryPct < 0) return;  // wasn't read this boot, skip

  int bx = 730;
  int by = 10;

  // battery outline
  display.drawRect(bx, by, 40, 18, GxEPD_BLACK);
  // terminal nub
  display.fillRect(bx + 40, by + 5, 4, 8, GxEPD_BLACK);

  if (batteryPct <= 15) {
    // just draw an X inside the battery and print LOW
    display.drawLine(bx + 4, by + 3, bx + 36, by + 14, GxEPD_BLACK);
    display.drawLine(bx + 36, by + 3, bx + 4, by + 14, GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(bx - 10, by + 30);
    display.print("LOW");
  } else {
    // fill bar proportionally
    int fillW = map(batteryPct, 0, 100, 0, 36);
    display.fillRect(bx + 2, by + 2, fillW, 14, GxEPD_BLACK);
    // percentage label underneath
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(bx + 4, by + 30);
    display.printf("%d%%", batteryPct);
  }
}

void drawWeatherIcon(int x, int y, String description) {
  description.toLowerCase();
  if (description.indexOf("clear") >= 0 || description.indexOf("sun") >= 0) {
    display.drawCircle(x + 10, y, 8, GxEPD_BLACK);
    display.drawLine(x + 10, y - 12, x + 10, y - 16, GxEPD_BLACK);
    display.drawLine(x + 10, y + 12, x + 10, y + 16, GxEPD_BLACK);
    display.drawLine(x - 2, y, x - 6, y, GxEPD_BLACK);
    display.drawLine(x + 22, y, x + 26, y, GxEPD_BLACK);
    display.drawLine(x - 2, y - 10, x - 5, y - 13, GxEPD_BLACK);
    display.drawLine(x + 22, y - 10, x + 25, y - 13, GxEPD_BLACK);
    display.drawLine(x - 2, y + 10, x - 5, y + 13, GxEPD_BLACK);
    display.drawLine(x + 22, y + 10, x + 25, y + 13, GxEPD_BLACK);
  } else if (description.indexOf("cloud") >= 0 && description.indexOf("rain") < 0) {
    display.drawCircle(x + 8, y + 2, 6, GxEPD_BLACK);
    display.drawCircle(x + 15, y - 2, 8, GxEPD_BLACK);
    display.drawCircle(x + 22, y + 2, 6, GxEPD_BLACK);
    display.drawLine(x + 2, y + 8, x + 28, y + 8, GxEPD_BLACK);
  } else if (description.indexOf("rain") >= 0 || description.indexOf("drizzle") >= 0) {
    display.drawCircle(x + 8, y - 2, 6, GxEPD_BLACK);
    display.drawCircle(x + 15, y - 6, 8, GxEPD_BLACK);
    display.drawCircle(x + 22, y - 2, 6, GxEPD_BLACK);
    display.drawLine(x + 2, y + 4, x + 28, y + 4, GxEPD_BLACK);
    display.drawLine(x + 8, y + 8, x + 6, y + 14, GxEPD_BLACK);
    display.drawLine(x + 15, y + 8, x + 13, y + 14, GxEPD_BLACK);
    display.drawLine(x + 22, y + 8, x + 20, y + 14, GxEPD_BLACK);
  } else if (description.indexOf("snow") >= 0) {
    display.drawLine(x + 10, y - 10, x + 10, y + 10, GxEPD_BLACK);
    display.drawLine(x, y, x + 20, y, GxEPD_BLACK);
    display.drawLine(x + 2, y - 8, x + 18, y + 8, GxEPD_BLACK);
    display.drawLine(x + 18, y - 8, x + 2, y + 8, GxEPD_BLACK);
  } else if (description.indexOf("thunder") >= 0 || description.indexOf("storm") >= 0) {
    display.drawLine(x + 14, y - 10, x + 6, y + 2, GxEPD_BLACK);
    display.drawLine(x + 6, y + 2, x + 12, y + 2, GxEPD_BLACK);
    display.drawLine(x + 12, y + 2, x + 4, y + 14, GxEPD_BLACK);
  } else {
    display.drawLine(x, y - 4, x + 20, y - 4, GxEPD_BLACK);
    display.drawLine(x + 2, y, x + 22, y, GxEPD_BLACK);
    display.drawLine(x, y + 4, x + 20, y + 4, GxEPD_BLACK);
  }
}

void drawHeader() {
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(10, 35);
  display.print(currentDateStr);
  display.setFont(&FreeMonoBold9pt7b);
  String viewLabel[] = {"WEEK", "DAY", "AGENDA"};
  display.setCursor(680, 20);
  display.print(viewLabel[currentView]);
  drawBatteryIndicator();
  display.drawLine(0, 45, 800, 45, GxEPD_BLACK);
}

void drawWeather() {
  drawWeatherIcon(10, 62, weather.description);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(45, 68);
  display.printf("%.0f°  H:%.0f°  L:%.0f°  Humidity: %d%%",
    weather.tempNow, weather.tempHigh, weather.tempLow, weather.humidity);
  display.drawLine(0, 75, 800, 75, GxEPD_BLACK);
}

void drawFooter() {
  display.drawLine(0, 455, 800, 455, GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 472);
  String q = dailyQuote;
  if (q.length() > 110) q = q.substring(0, 107) + "...";
  display.print(q);
}

void drawFocusMode() {
  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeInfo);
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(290, 180);
  display.print(timeStr);
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(260, 220);
  display.print(currentDateStr);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(200, 300);
  String q = dailyQuote;
  if (q.length() > 80) q = q.substring(0, 77) + "...";
  display.print(q);
  display.setCursor(300, 360);
  display.print("No events today.");
}

void drawWeekView() {
  display.setFont(&FreeMonoBold9pt7b);
  const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  int colW = 800 / 7;
  int y = 83;
  for (int i = 0; i < 7; i++) {
    int x = i * colW;
    display.setCursor(x + 5, y + 15);
    display.print(days[i]);
    if (i == timeInfo.tm_wday) {
      display.drawRect(x, y, colW, 20, GxEPD_BLACK);
    }
  }
  int evY[7] = {y + 25, y + 25, y + 25, y + 25, y + 25, y + 25, y + 25};
  for (int i = 0; i < eventCount; i++) {
    int eventDow = getDayOfWeek(events[i].date);
    if (eventDow < 0) continue;
    int x = eventDow * colW + 2;
    if (evY[eventDow] < 450) {
      display.setCursor(x, evY[eventDow]);
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
  for (int i = 1; i < 7; i++) {
    display.drawLine(i * colW, y, i * colW, 450, GxEPD_BLACK);
  }
}

void drawDayView() {
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(10, 100);
  display.print("Today");
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
    display.print("Nothing scheduled today.");
  }
}

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

void renderDisplay() {
  bool noEventsToday = true;
  char todayStr[11];
  strftime(todayStr, sizeof(todayStr), "%Y-%m-%d", &timeInfo);
  for (int i = 0; i < eventCount; i++) {
    if (events[i].date == String(todayStr)) {
      noEventsToday = false;
      break;
    }
  }

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    drawHeader();
    drawWeather();

    if (cfg_focusMode && noEventsToday) {
      drawFocusMode();
    } else {
      switch (currentView) {
        case 0: drawWeekView();   break;
        case 1: drawDayView();    break;
        case 2: drawAgendaView(); break;
      }
    }

    drawFooter();
  } while (display.nextPage());
}


// ─── helpers ──────────────────────────────────────────────────────────────────

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
  int diff = t.tm_yday - timeInfo.tm_yday;
  if (diff < -timeInfo.tm_wday || diff > (6 - timeInfo.tm_wday)) return -1;
  return t.tm_wday;
}
