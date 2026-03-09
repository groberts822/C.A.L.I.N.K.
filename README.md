# CALINK
### Calendar Ambient Low-power Ink Network Keeper

> A low-power e-ink wall calendar that keeps your schedule visible all day — glanceable, distraction-free, and always up to date.

<!---------------------------------------------------------------------------->
<!-- HERO IMAGE: Photo of the finished CALINK mounted on your wall, display  -->
<!-- showing a full week view with events, weather, and daily quote.          -->
<!-- Suggested: natural lighting, clean background, slightly angled shot.     -->
<!---------------------------------------------------------------------------->
![CALINK Mounted on Wall](docs/images/hero.jpg)

---

## What is CALINK?

CALINK is a wall-mounted e-ink calendar display powered by an ESP32. It syncs with your Google Calendar over WiFi, shows the weather, and displays a daily quote — all on a 7.5 inch black and white e-ink screen that barely uses any battery. Because e-ink only draws power when the image changes, CALINK can run for weeks on a single charge while always showing your schedule.

The idea came from wanting something simple on my wall — not a screen that glows, not a phone I have to pick up, just a calm always-visible display that tells me what's happening today and this week without asking for my attention. I wanted it to feel more like a piece of paper than a gadget.

---

## Features

- Google Calendar sync — pulls your real events automatically
- Weather — current temp, high, low, and conditions
- Daily quote — a new one every refresh
- Three views — Week, Day, and Agenda — cycles on each refresh
- Deep sleep — ESP32 sleeps between refreshes, battery lasts weeks
- NTP time sync — always accurate, no RTC module needed
- 3D printed enclosure — custom designed to hang flush on the wall

---

## Hardware

### The Display — Waveshare 7.5" e-Paper (800×480, B/W)

<!---------------------------------------------------------------------------->
<!-- IMAGE: Close-up of the Waveshare 7.5" raw e-ink display panel.          -->
<!-- Suggested: flat lay on a white surface showing the panel and ribbon      -->
<!-- cable. Show the ZIF connector end clearly.                               -->
<!---------------------------------------------------------------------------->
![Waveshare 7.5" e-ink display](docs/images/display.jpg)

I chose the **Waveshare 7.5 inch raw B/W e-ink panel** for a few reasons. At 800×480 pixels it is big enough to show a full week view with readable text, but not so large that it dominates a wall. I went with black and white only — not the red/yellow color version — because the B/W panel refreshes much faster and has better contrast for text. Color e-ink panels can take 30+ seconds to refresh, which felt too slow even for a calendar.

The "raw panel" version has no driver board built in, just the display glass and a flat ribbon cable. This keeps it thin and light, and lets the driver board handle all the logic.

**Specs:**
- Resolution: 800 x 480
- Size: 7.5 inches
- Colors: Black and white
- Interface: SPI
- Refresh time: ~2 seconds full refresh

---

### The Brains — Waveshare Universal e-Paper Driver Board (ESP32)

<!---------------------------------------------------------------------------->
<!-- IMAGE: The Waveshare ESP32 driver board on its own, showing the ZIF     -->
<!-- connector, ESP32 chip, and WiFi antenna clearly.                         -->
<!-- Suggested: flat lay, good lighting, show both sides if interesting.      -->
<!---------------------------------------------------------------------------->
![Waveshare ESP32 Driver Board](docs/images/driver_board.jpg)

The **Waveshare Universal e-Paper Driver Board** is the core of CALINK. It does three jobs at once: it drives the e-ink display via a built-in ZIF/FPC latch connector, it runs the ESP32 microcontroller that handles all the code, and it provides WiFi for calendar and weather sync. Connecting the display is as simple as flipping the latch, sliding the ribbon cable in, and pressing the latch back down — no soldering required for the display connection at all.

I chose this board over wiring up a bare ESP32 because it eliminates a huge amount of complexity. The e-ink display requires specific driver circuitry and precise SPI timing — Waveshare has already solved all of that. Using their driver board meant I could focus on the software and the enclosure instead of debugging display signals.

**Specs:**
- MCU: ESP32 dual core 240MHz
- WiFi: 802.11 b/g/n
- Bluetooth: 4.2
- Display interface: ZIF/FPC latch, universal, fits all Waveshare panels
- Power input: 3.6V to 5.5V

---

### Power — LiPo Battery + TP4056 Charger

<!---------------------------------------------------------------------------->
<!-- IMAGE: The LiPo battery and TP4056 module side by side.                 -->
<!-- Suggested: flat lay showing both components with the JST connector       -->
<!-- visible on the battery and the USB-C port on the TP4056.                -->
<!---------------------------------------------------------------------------->
![LiPo battery and TP4056 charger](docs/images/power.jpg)

For power I used a **1100mAh LiPo battery** paired with a **TP4056 USB-C charging module**. The ESP32 deep sleep mode draws almost no power between refreshes, so a 1100mAh cell lasts for weeks at a 1-hour refresh interval.

The TP4056 handles charging safely — it has built-in overcharge and overdischarge protection and charges the battery via USB-C. In the enclosure the USB-C port is accessible from the side so CALINK can be charged without taking it off the wall.

**Wiring:**
```
LiPo B+      →  TP4056 B+
LiPo B-      →  TP4056 B-
TP4056 OUT+  →  ESP32 Driver Board 5V pin
TP4056 OUT-  →  ESP32 Driver Board GND
```

---

### Why No Custom PCB?

<!---------------------------------------------------------------------------->
<!-- IMAGE: All components laid out together showing how they connect —       -->
<!-- display, driver board, battery, and TP4056.                              -->
<!-- Suggested: flat lay bird's eye view, slightly spread out so all parts    -->
<!-- are visible and identifiable.                                            -->
<!---------------------------------------------------------------------------->
![All components laid out](docs/images/components_layout.jpg)

Early in the project I planned to design a custom PCB to tie everything together. After mapping out what the PCB would actually do, I realized it would just be a power distribution board with four connectors — not meaningfully adding to the project. The connections between components are already clean: the display ribbon plugs directly into the driver board ZIF connector, the battery and TP4056 connect with JST plugs, and the whole thing fits neatly inside the 3D printed enclosure.

Skipping the PCB kept the project simpler, cheaper, and easier to assemble. The real complexity lives where it matters — in the software and the enclosure design.

---

### The Enclosure — 3D Printed

<!---------------------------------------------------------------------------->
<!-- IMAGE: The 3D printed enclosure before assembly — show front, back,     -->
<!-- and side in three separate photos.                                       -->
<!-- Suggested:                                                               -->
<!--   enclosure_front.jpg — front face showing display cutout               -->
<!--   enclosure_back.jpg  — back showing wall mount keyhole slots           -->
<!--   enclosure_side.jpg  — side showing USB-C cutout                       -->
<!---------------------------------------------------------------------------->
![3D Printed Enclosure Front](docs/images/enclosure_front.jpg)
![Enclosure Back with Wall Mount](docs/images/enclosure_back.jpg)
![Enclosure Side Profile](docs/images/enclosure_side.jpg)

The enclosure was designed in CAD and 3D printed. The goals were: as thin as possible, flush to the wall, and the USB-C charging port accessible from the side without removing the device. The display sits in a recessed front frame so it appears flush with the face, and the whole unit hangs on two wall anchors from keyhole slots on the back.

<!---------------------------------------------------------------------------->
<!-- IMAGE: CAD screenshot or render showing the internal layout with        -->
<!-- components placed inside.                                                -->
<!-- Suggested: Fusion 360 / OnShape render or screenshot from the CAD tool  -->
<!---------------------------------------------------------------------------->
![CAD Design](docs/images/cad_render.jpg)

**Design decisions:**
- Recessed display frame so the screen sits flush with the front face
- Side cutout for USB-C charging access while mounted
- Internal channels to route wires cleanly
- Two keyhole slots on the back for tool-free wall mounting
- Snap-fit back panel for easy access without screws

---

## Assembly

<!---------------------------------------------------------------------------->
<!-- IMAGES: Step by step assembly — aim for 5 photos:                       -->
<!--   assembly_1.jpg — components next to open enclosure                    -->
<!--   assembly_2.jpg — driver board seated in enclosure                     -->
<!--   assembly_3.jpg — display ribbon cable being inserted into ZIF latch   -->
<!--   assembly_4.jpg — battery and TP4056 wired and tucked in               -->
<!--   assembly_5.jpg — finished unit, back panel on                         -->
<!---------------------------------------------------------------------------->
![Assembly Step 1](docs/images/assembly_1.jpg)
![Assembly Step 2](docs/images/assembly_2.jpg)
![Assembly Step 3](docs/images/assembly_3.jpg)
![Assembly Step 4](docs/images/assembly_4.jpg)
![Assembly Step 5](docs/images/assembly_5.jpg)

1. Seat the ESP32 driver board in the enclosure
2. Flip the ZIF latch on the driver board, slide the display ribbon cable in, press latch down
3. Wire TP4056 OUT+ and OUT- to the driver board 5V and GND pins
4. Connect the LiPo battery JST plug to the TP4056
5. Tuck the battery and TP4056 into the enclosure cavity
6. Snap the back panel on
7. Mount on wall

---

## Software Setup

### 1. Install Arduino Libraries

In Arduino IDE go to **Tools → Manage Libraries** and install:
- `GxEPD2` by ZinggJM
- `ArduinoJson` by Benoit Blanchon

Install ESP32 board support — File → Preferences → add to Additional Board URLs:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then Tools → Board → Boards Manager → search `esp32` → install.

---

### 2. Set Up Google Apps Script

CALINK fetches your Google Calendar through a Google Apps Script web app. It is free and requires no server.

1. Go to [script.google.com](https://script.google.com) and create a new project
2. Paste this code:

```javascript
function doGet() {
  var calendar = CalendarApp.getDefaultCalendar();
  var now = new Date();
  var end = new Date();
  end.setDate(end.getDate() + 30);

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

---

### 3. Get OpenWeatherMap API Key

Sign up free at [openweathermap.org](https://openweathermap.org/api) and copy your key from the API Keys page.

---

### 4. Configure calink.ino

Open `calink.ino` and fill in the config section at the top:

```cpp
#define WIFI_SSID         "your_wifi_name"
#define WIFI_PASSWORD     "your_wifi_password"
#define GAS_URL           "https://script.google.com/macros/s/YOUR_ID/exec"
#define OWM_API_KEY       "your_openweathermap_key"
#define OWM_CITY          "Your City"
#define GMT_OFFSET_SEC    -18000   // EST = -5 x 3600. Adjust for your timezone.
```

---

### 5. Upload

- Connect the driver board via USB-C
- Tools → Board → **ESP32 Dev Module**
- Tools → Port → select your port
- Click Upload

---

## Changing the Refresh Rate

In `calink.ino` change this value in seconds:

```cpp
#define SLEEP_SECONDS  3600  // refresh every 1 hour
```

Lower = more frequent updates but shorter battery life. At 1 hour, expect several weeks per charge.

---

## Views

CALINK cycles through three views automatically on each refresh:

| View | Description |
|---|---|
| **Week** | Full 7-day grid with events under each day |
| **Day** | Today's events listed in detail with times |
| **Agenda** | All upcoming events in chronological order |

<!---------------------------------------------------------------------------->
<!-- IMAGES: One photo of the display showing each view.                     -->
<!--   view_week.jpg   — week grid view                                      -->
<!--   view_day.jpg    — day view with today's events                        -->
<!--   view_agenda.jpg — agenda list view                                    -->
<!---------------------------------------------------------------------------->
![Week View](docs/images/view_week.jpg)
![Day View](docs/images/view_day.jpg)
![Agenda View](docs/images/view_agenda.jpg)

---

## Bill of Materials

| Part | Distributor | Qty |
|---|---|---|
| Waveshare 7.5" e-Paper 800x480 B/W raw panel | Waveshare / Amazon | 1 |
| Waveshare Universal e-Paper Driver Board (ESP32) | Waveshare / Amazon | 1 |
| MakerFocus 1100mAh LiPo JST 1.25 (4-pack) | Amazon | 1 pack |
| TP4056 USB-C LiPo Charger Module | Amazon | 1 |

---

## License

MIT — free to use, modify, and share.

---

*Built for Hack Club High Seas*
