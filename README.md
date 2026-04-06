# CALINK
### Calendar Ambient Low-power Ink Network Keeper

> A low-power e-ink wall calendar that keeps your schedule visible all day — glanceable, distraction-free, and always up to date.

---

## What is CALINK?

CALINK is a wall-mounted e-ink calendar display powered by an ESP32. It syncs with your Google Calendar over WiFi, shows the weather, and displays a daily quote. This all takes place on a 7.5 inch black and white e-ink screen that uses a little amount of battery. Because e-ink only draws power when the image changes, CALINK can run for weeks on a single charge while always showing stuff.

The idea came from wanting a calm, always-on display that tells me what's happening today and this week without asking for my attention. I wanted it to feel more like the kindles do rather than a monitor.

---

## Features

- Google Calendar sync — pulls your real events automatically
- Weather — current temp, high, low, and conditions
- Daily quote — a new one every refresh
- Three views — Week, Day, and Agenda — cycles on each refresh
- Deep sleep — ESP32 sleeps between refreshes, battery lasts weeks
- NTP time sync — always accurate, no RTC module needed
- 3D printed enclosure — custom designed to hang flush on the wall
- Custom boot screen — CALINK logo displays on startup
- Focus mode — unique layout when you have nothing scheduled today
- Custom pixel art weather icons — hand drawn for e-ink
- Custom display layout — fully original typography and grid design

---

## Hardware

### The Display — Waveshare 7.5" e-Paper (800×480, B/W)

<img width="431" height="353" alt="image" src="https://github.com/user-attachments/assets/052fd2c4-ca41-4cf7-a37e-5ef4aa8200c5" />

I chose the **Waveshare 7.5 inch raw B/W e-ink panel** for a few reasons. At 800×480 pixels it is big enough to show a full week view with readable text, but not so large that it covers a wall. I went with black and white only (there's ones with yellow and red too) because the B/W panel refreshes much faster and has better contrast for text. Color e-ink panels can take 30+ seconds to refresh, which felt too slow even for a calendar.

The "raw panel" version has no driver board built in, just the display glass and a flat ribbon cable. This keeps it thin and light, and lets the esp32 driver board handle it all.

**Specs:**
- Resolution: 800 x 480
- Size: 7.5 inches
- Colors: Black and white
- Interface: SPI
- Refresh time: ~2 seconds full refresh

---

### The Brains — Waveshare Universal e-Paper Driver Board (ESP32)

<img width="381" height="316" alt="image" src="https://github.com/user-attachments/assets/b87a273d-5673-4f94-8196-383726db96b4" />

The **Waveshare Universal e-Paper Driver Board** is the microcontroller for controlling CALINK. It has 3 jobs: it drives the e-ink display via a built-in ZIF/FPC latch connector, it runs the ESP32 microcontroller that handles all the code, and it provides WiFi for calendar and weather sync. Connecting the display is as simple as flipping the latch, sliding the ribbon cable in, and pressing the latch back down — no soldering required.

I chose this board over wiring up a bare ESP32 because it eliminates a huge amount of complexity. The e-ink display requires specific driver circuitry and precise SPI timing, and Waveshare has already solved all of that for us. Using their driver board meant I could focus on the software and the enclosure instead of spending my time debugging.

**Specs:**
- MCU: ESP32 dual core 240MHz
- WiFi: 802.11 b/g/n
- Bluetooth: 4.2
- Display interface: ZIF/FPC latch, universal, fits all Waveshare panels
- Power input: 3.6V to 5.5V

---

### Power — LiPo Battery + TP4056 Charger

<img width="458" height="344" alt="image" src="https://github.com/user-attachments/assets/6c534fe2-5d70-4042-8fdf-f7f046002b33" />

<img width="457" height="369" alt="image" src="https://github.com/user-attachments/assets/86c7fd98-84c6-479d-beca-c4d2f64e56fe" />

For power I used a **1100mAh LiPo battery** paired with a **TP4056 USB-C charging module**. The ESP32 deep sleep mode draws almost no power between refreshes, so a 1100mAh cell lasts for weeks at a 1-hour refresh interval.

The TP4056 handles charging safely — it has built-in overcharge and overdischarge protection and charges via USB-C. In the enclosure the USB-C port is accessible from the bottom so CALINK can be charged without taking it off the wall.

**Wiring:**
```
LiPo B+      →  TP4056 B+
LiPo B-      →  TP4056 B-
TP4056 OUT+  →  Slide Switch (Pin 1)
TP4056 OUT-  →  ESP32 Driver Board GND
Slide Switch (Pin 2)  →  ESP32 Driver Board 5V pin
```

---

### Why No Custom PCB?

<img width="839" height="614" alt="image" src="https://github.com/user-attachments/assets/5e68f6f6-e9f7-4519-ba34-3c5fb1c33673" />

Early in the project I planned to design a custom PCB to tie everything together. After mapping out what the PCB would actually do, I realized it would just be a power distribution board with four connectors — not meaningfully adding to the project. The connections between components are already clean: the display ribbon plugs directly into the driver board ZIF connector, the battery and TP4056 connect with JST plugs, and the whole thing fits neatly inside the 3D printed enclosure.

Skipping the PCB kept the project simpler, cheaper, and just easier.

However, below is the wiring diagram for our project, it is quite simple: 

<img width="1079" height="610" alt="image" src="https://github.com/user-attachments/assets/16fafd9a-6ece-44f4-87b1-bde0ce97f993" />

Just to note, the power lines going from the switch/tp4056 into the esp32 board powers the board via VIN and GND.

---

### The Enclosure — 3D Printed

<img width="808" height="553" alt="image" src="https://github.com/user-attachments/assets/64bc9b8c-cb33-49e3-852f-a47eff887f8d" />

<img width="612" height="438" alt="image" src="https://github.com/user-attachments/assets/2a958ed1-56d6-4414-ab90-03f40801991d" />

<img width="613" height="91" alt="image" src="https://github.com/user-attachments/assets/7bc8a3e3-9fda-4cac-a896-a88c183f8c86" />

The enclosure was designed in CAD and 3D printed. The goals were to keep it as thin as possible, flush to the wall, and the USB-C charging port accessible from the side without removing the device. The display sits in a recessed front frame so it appears flush with the face, and the whole unit hangs on two wall anchors from keyhole slots on the back.

**Design decisions:**
- Recessed display frame so the screen sits flush with the front face
- Side cutout for USB-C charging access while mounted
- Internal channels to route wires cleanly
- Two keyhole slots on the back for tool-free wall mounting
- Snap-fit back panel for easy access without screws

---

## Assembly

<img width="1129" height="662" alt="image" src="https://github.com/user-attachments/assets/598ea644-6d53-462b-90d6-c9ad379b1020" />

Below is a video scrolling through and showing the proper connections the model makes. 

```
https://youtu.be/8ukgVpSq_vM
```

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
| **Focus** | Shown when no events today — large clock, quote, and weather |

---

## Design

CALINK has a fully custom display layout, it does not rely on a default font or generic grid. The text and visuals were designed specifically for e-ink rendering, and the weather conditions are shown as hand-drawn pixel art icons rather than text. When CALINK boots up, a custom boot screen displays the CALINK logo before the calendar loads.

---

## Bill of Materials

| Part | Distributor | Qty |
|---|---|---|
| [Waveshare 7.5" e-Paper 800x480 B/W raw panel](https://www.waveshare.com/product/7.5inch-e-paper.htm) | Waveshare | 1 |
| [Waveshare Universal e-Paper Driver Board (ESP32)](https://www.waveshare.com/e-paper-esp32-driver-board.htm) | Waveshare | 1 |
| [MakerHawk 1100mAh LiPo JST 1.25](https://www.amazon.com/MakerHawk-Rechargeable-Protection-Insulated-Development/dp/B0D7MC714N/ref=sr_1_9?crid=2QHZH5SEEDYN3&dib=eyJ2IjoiMSJ9.oq5w-uttgpXKQrnlBGwtJ7NU31KarYm8Jmxs9vnhJyuVjAJCX2VTRqWx9SVWf4VUyyKxCwqegoRywSLQialBJgKem-ChqyYNFevoyECAiweDuJhLfvDO6vBXyLveYJd3KO9n4gGAwZcxphNePJpMVUYtAy01E-v0b-Dql8afNFxMXC_Iq88ENMkAZ66oKAA9bZ0F8oWL11-7LT9aXu2k6ckZ2L6N1JHDLiA1_sbCOF10Y5OLwU5nAZb2aKnbIWTu87jgxaoQ2XC3Zo5O-tZwcW_RthrBQSuUGPjPLSY7Drc.VvCsYUT93cTPTNgag2eT2-HLpPupL5AT9QnipHAmgoE&dib_tag=se&keywords=1100%2Bmah%2B4.7v%2Blipo%2Bbattery&qid=1775519180&sprefix=1100%2Bmah%2B4.7v%2Blipo%2Bbattery%2Caps%2C115&sr=8-9&th=1) | Amazon | 1 pack |
| [TP4056 USB-C LiPo Charger Module](https://www.aliexpress.us/item/3256808741161726.html?spm=a2g0o.productlist.main.16.4961AksiAksicO&algo_pvid=e604797a-f0bb-4e90-8345-5deb917db8a0&algo_exp_id=e604797a-f0bb-4e90-8345-5deb917db8a0-13&pdp_ext_f=%7B"order"%3A"317"%2C"eval"%3A"1"%2C"fromPage"%3A"search"%7D&pdp_npi=6%40dis%21USD%211.61%211.59%21%21%2111.06%2110.90%21%402101eecd17755184633537336e990d%2112000047233885048%21sea%21US%217425461850%21X%211%210%21n_tag%3A-29913%3Bd%3Abcfded55%3Bm03_new_user%3A-29895&curPageLogUid=D319WdCvBSxz&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005008927476478%7C_p_origin_prod%3A) | Aliexpress | 1 |
| [SPDT Toggle Switch](https://www.aliexpress.us/item/3256805322448598.html?spm=a2g0o.productlist.main.9.b211c5kdc5kdo6&algo_pvid=282c5e60-984c-4ca7-8133-2d3da957b021&algo_exp_id=282c5e60-984c-4ca7-8133-2d3da957b021-8&pdp_ext_f=%7B"order"%3A"360"%2C"eval"%3A"1"%2C"fromPage"%3A"search"%7D&pdp_npi=6%40dis%21USD%211.33%211.29%21%21%211.33%211.29%21%40210328df17755183955124138ee6c5%2112000033353662261%21sea%21US%217425461850%21X%211%210%21n_tag%3A-29913%3Bd%3Abcfded55%3Bm03_new_user%3A-29895&curPageLogUid=PR3GyGcTbi8d&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005005508763350%7C_p_origin_prod%3A) | Aliexpress | 1 |
| [100kohm Resistor](https://www.aliexpress.us/item/3256806712416689.html?spm=a2g0o.productlist.main.5.19152327skyPcY&algo_pvid=dd811bbb-00d6-4929-bf3b-33b4c6a984df&algo_exp_id=dd811bbb-00d6-4929-bf3b-33b4c6a984df-4&pdp_ext_f=%7B"order"%3A"16417"%2C"spu_best_type"%3A"price"%2C"eval"%3A"1"%2C"fromPage"%3A"search"%7D&pdp_npi=6%40dis%21USD%211.51%210.96%21%21%2110.35%216.59%21%40210311c217755181716484983e5e2f%2112000038655414005%21sea%21US%217425461850%21X%211%210%21n_tag%3A-29913%3Bd%3A2f50d3e6%3Bm03_new_user%3A-29895%3BpisId%3A5000000203831334&curPageLogUid=hMLmxlgf0ZGM&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005006898731441%7C_p_origin_prod%3A&_gl=1*1fby3wl*_gcl_aw*R0NMLjE3NjkwMzU1NzEuRUFJYUlRb2JDaE1JdDY3dHF0cWRrZ01WbW1oSEFSMG93UWdMRUFRWUFpQUJFZ0tiV19EX0J3RQ..*_gcl_dc*R0NMLjE3NjkwMzU1NzEuRUFJYUlRb2JDaE1JdDY3dHF0cWRrZ01WbW1oSEFSMG93UWdMRUFRWUFpQUJFZ0tiV19EX0J3RQ..*_gcl_au*OTExMzQxMzAyLjE3NzM2MTUyNjg.*_ga*MTY2NjQwMzk4OC4xNzczNjE1MjY4*_ga_VED1YSGNC7*czE3NzU1MTcxMjckbzUkZzEkdDE3NzU1MTgxNzAkajU0JGwwJGgw) | Aliexpress | 1 |

---
NOT CREATED by AI :(
