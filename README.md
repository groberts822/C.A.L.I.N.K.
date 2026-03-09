# C.A.L.I.N.K.
# CALINK
### Calendar Ambient Low-power Ink Network Keeper

> A low-power e-ink wall calendar that keeps your schedule visible all day — glanceable, distraction-free, and always up to date.

![CALINK Banner](docs/banner.png)

---

## Hardware

| Part | Description |
|---|---|
| Waveshare 7.5" e-Paper 800×480 B/W (raw panel) | Main display |
| Waveshare Universal e-Paper Driver Board (ESP32) | Display driver + ESP32 + WiFi |
| MakerFocus 1100mAh LiPo JST 1.25 | Battery |
| TP4056 USB-C LiPo Charger Module | Charging |

---

## Features

- **Google Calendar sync** — pulls your events via Google Apps Script
- **Weather** — current conditions via OpenWeatherMap (free tier)
- **Daily quote** — random inspirational quote via quotable.io
- **3 views** — Week, Day, and Agenda — cycles automatically on each refresh
- **Deep sleep** — ESP32 sleeps between refreshes for maximum battery life
- **NTP time sync** — always accurate, no RTC module needed

---

## Setup

### 1. Install Arduino Libraries

In Arduino IDE, go to **Tools → Manage Libraries** and install:

- `GxEPD2` by ZinggJM
- `ArduinoJson` by Benoit Blanchon

Also install the **ESP32 board support**:
- File → Preferences → add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to Additional Board URLs
- Tools → Board → Boards Manager → search `esp32` → install

### 2. Set Up Google Apps Script

This is how CALINK fetches your Google Calendar — it's free and requires no server.

1. Go to [script.google.com](https://script.google.com) and create a new project
2. Paste the following code:

```javascript
function doGet() {
  var calendar = CalendarApp.getDefaultCalendar();
  var now = new Date();
  var end = new Date();
  end.setDate(end.getDate() + 30); // fetch next 30 days

  var events = calendar.getEvents(now, end);
  var result = [];

  events.forEach(function(e) {
    result.push({
      title: e.getTitle(),
      date: Utilities.formatDate(e.getStartTime(), Session.getScriptTimeZone(), "yyyy-MM-dd"),
      start: e.getStartTime().toISOString(),
      end: e.getEndTime().toISOString(),
      allDay: e.isAllDayEvent()
    });
  });

  return ContentService
    .createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}
```

3. Click **Deploy → New Deployment → Web App**
4. Set **Execute as: Me** and **Who has access: Anyone**
5. Copy the web app URL — this is your `GAS_URL`

### 3. Get OpenWeatherMap API Key

1. Sign up free at [openweathermap.org](https://openweathermap.org/api)
2. Go to API Keys and copy your key

### 4. Configure calink.ino

Open `calink.ino` and fill in the config section at the top:

```cpp
#define WIFI_SSID         "your_wifi_name"
#define WIFI_PASSWORD     "your_wifi_password"
#define GAS_URL           "https://script.google.com/macros/s/YOUR_ID/exec"
#define OWM_API_KEY       "your_openweathermap_key"
#define OWM_CITY          "Your City"
#define GMT_OFFSET_SEC    -18000   // EST = -5h × 3600
```

### 5. Upload

- Connect the Waveshare ESP32 Driver Board via USB-C
- Tools → Board → **ESP32 Dev Module**
- Tools → Port → select your port
- Click **Upload**

---

## Wiring

The raw e-Paper display connects directly to the Waveshare ESP32 Driver Board via the onboard ZIF/FPC latch connector — no soldering required. Flip the latch up, slide the ribbon cable in, press the latch down.

**Power:**
```
LiPo B+  →  TP4056 B+
LiPo B-  →  TP4056 B-
TP4056 OUT+  →  ESP32 Driver Board 5V
TP4056 OUT-  →  ESP32 Driver Board GND
```

---

## Changing Refresh Rate

In `calink.ino`, change this value (in seconds):

```cpp
#define SLEEP_SECONDS  3600  // 1 hour
```

Lower = more frequent updates but shorter battery life.
At 1 hour refresh, expect **weeks** of battery life from the 1100mAh LiPo.

---

## Changing Timezone

Find your UTC offset in seconds and update:

```cpp
#define GMT_OFFSET_SEC  -18000  // EST (UTC-5)
#define DAYLIGHT_OFFSET  3600   // 1hr DST offset (set 0 if no DST)
```

---

## Views

CALINK automatically cycles through 3 views on each refresh:

| View | Description |
|---|---|
| **Week** | Full 7-day week grid with events |
| **Day** | Today's events in detail |
| **Agenda** | All upcoming events listed chronologically |

---

## License

MIT License — free to use, modify, and share.

---

## Built for Hack Club Highseas
