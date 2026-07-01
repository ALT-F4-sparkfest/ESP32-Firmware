# ESP32 Firmware — Technical Documentation
**Project:** Busina (Smart Route) — Real-time Public Transportation Telemetry
**Repository:** [ALT-F4-sparkfest/ESP32-Firmware](https://github.com/ALT-F4-sparkfest/ESP32-Firmware)
**Platform:** PlatformIO / Arduino framework, `esp32dev` board

---

## 1. Overview

This firmware runs on an ESP32 microcontroller mounted on a jeepney (public transit vehicle) as part of **Busina**, a real-time public transportation platform. The device's job is to determine whether the vehicle is currently moving, read its GPS position, and publish that telemetry over MQTT so it can be consumed by a live-tracking / ETA-prediction backend.

At a high level, the firmware:

1. Connects to Wi-Fi using a captive-portal configuration flow (no hard-coded credentials).
2. Reads a vibration sensor to infer whether the jeepney is running or idle.
3. Reads NMEA sentences from a GPS module and extracts latitude, longitude, and speed.
4. Publishes a JSON telemetry payload to an MQTT topic at a fixed interval, but only while the vehicle is "active" and has a valid GPS fix.

---

## 2. Repository Structure

```
ESP32-Firmware/
├── platformio.ini          # Build configuration and dependencies
├── src/
│   └── main.cpp             # Application entry point (setup/loop)
├── lib/
│   ├── gps/
│   │   ├── gps.h
│   │   └── gps.cpp          # GPS acquisition + JSON payload builder
│   ├── mqtt/
│   │   ├── mqtt.h
│   │   └── mqtt.cpp         # Wi-Fi + MQTT client handling
│   └── sens/
│       ├── sens.h
│       └── sens.cpp         # Vibration-sensor "is moving" detection
├── include/                  # (empty, PlatformIO convention)
├── test/                     # (empty, PlatformIO convention)
└── .vscode/                  # Editor configuration
```

Each hardware subsystem (GPS, MQTT/networking, motion sensing) is isolated into its own PlatformIO **library** under `lib/`, keeping `main.cpp` as a thin orchestrator.

---

## 3. Build Configuration (`platformio.ini`)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    mikalhart/TinyGPSPlus@^1.0.3     ; GPS NMEA parser (used with a Neo-6M module)
    knolleary/PubSubClient@^2.8      ; MQTT client
    tzapu/WiFiManager @ ^2.0.17      ; Wi-Fi captive-portal provisioning
```

| Setting | Value |
|---|---|
| Board | `esp32dev` (generic ESP32 dev board) |
| Framework | Arduino |
| Serial monitor baud rate | 115200 |
| GPS module | u-blox Neo-6M (or compatible), via `TinyGPS++` |
| MQTT library | `PubSubClient` |
| Wi-Fi provisioning | `WiFiManager` (captive portal, no hard-coded SSID/password) |

---

## 4. Hardware / Pin Mapping

| Peripheral | Interface | Pins / Notes |
|---|---|---|
| GPS module (Neo-6M) | UART2 (`HardwareSerial GPS_Serial(2)`) | RX = GPIO 16, TX = GPIO 17, 9600 baud |
| Vibration sensor | Digital input | GPIO 32 (`SEN_PIN`) |
| Wi-Fi | Built-in ESP32 radio | Configured via WiFiManager portal `SmartRoute_Setup` |

---

## 5. Module Reference

### 5.1 `src/main.cpp` — Application Entry Point

**Globals**
- `char vehicle_id[20]` — configurable identifier for the vehicle, defaults to `"jeep_default_01"`, editable through the Wi-Fi portal.
- `lastPublishTime`, `PUBLISH_INTERVAL` (3000 ms) — throttles how often telemetry is published.

**`setup()`**
1. Starts the serial console at 115200 baud.
2. Instantiates `WiFiManager` and registers a custom parameter field ("Vehicle ID") so the field appears in the captive-portal web form alongside the SSID/password prompts.
3. Calls `wm.autoConnect("SmartRoute_Setup")`. If a known network is saved, it connects automatically; otherwise it opens an access point named **`SmartRoute_Setup`** with a configuration webpage. If connection fails outright, the device reboots (`ESP.restart()`).
4. Copies the vehicle ID entered by the installer/technician into `vehicle_id`.
5. Initializes the vibration sensor (`initSens()`) and GPS module (`initGPS()`).

**`loop()`**
1. `maintainNetwork()` — keeps the MQTT connection alive (see §5.3).
2. `updateSensStatus()` — refreshes the "is the jeep moving" flag.
3. `updateGPS()` — feeds any newly-arrived NMEA bytes into the parser.
4. If the jeep is active **and** a valid GPS fix exists **and** at least `PUBLISH_INTERVAL` (3 s) has elapsed since the last publish, it builds a JSON payload via `getGPSTelemetry()` and publishes it to the topic `smartroute/vehicles/live`.

### 5.2 `lib/gps` — GPS Acquisition

**`gps.h`**
```cpp
void initGPS();
void updateGPS();
bool hasValidLocation();
String getGPSTelemetry(const char* vehicle_id);
```

**Behavior**
- Uses `TinyGPSPlus` to parse NMEA sentences streamed from `GPS_Serial` (UART2 @ 9600 baud).
- `updateGPS()` should be called every loop iteration to drain the serial buffer byte-by-byte into the parser.
- `hasValidLocation()` returns `true` only when the parser reports both an **updated** and a **valid** fix (guards against stale or corrupt readings).
- `getGPSTelemetry()` manually builds a compact JSON string (rather than using a JSON library, to conserve RAM/flash) with the shape:

```json
{"id":"jeep_default_01","lat":14.676000,"lng":121.043500,"spd":23.40}
```

  - `lat` / `lng`: 6 decimal places (~11 cm precision)
  - `spd`: speed over ground in km/h, 2 decimal places

### 5.3 `lib/mqtt` — Networking & Telemetry Publishing

**`mqtt.h`**
```cpp
void initMQTT(const char* ssid, const char* password, const char* mqtt_server);
void maintainNetwork();
void publishTelemetry(const char* topic, String payload);
```

**Behavior**
- `initMQTT()` connects to a given Wi-Fi SSID/password and sets the MQTT broker host on **port 1883** (the standard unencrypted MQTT port).
- `maintainNetwork()` is a blocking reconnect loop: while the `PubSubClient` is disconnected, it repeatedly attempts to connect using a randomized client ID of the form `SmartRoute-<hex>`, retrying every 5 seconds. Once connected it calls `client.loop()` to service the MQTT session.
- `publishTelemetry()` publishes a string payload to an arbitrary topic and logs it to the serial console.

> **⚠️ Integration note:** `initMQTT()` is the only place in the codebase that calls `client.setServer(...)` to configure the MQTT broker address, but `main.cpp` never calls `initMQTT()` — Wi-Fi connection was migrated to `WiFiManager`, and the broker configuration step appears to have been left out of that migration. As shipped, `maintainNetwork()` will attempt to connect to an MQTT broker that was never assigned a host/port, so telemetry publishing will not reach a broker until either `client.setServer()` is called elsewhere (e.g., after `wm.autoConnect()`, ideally using another `WiFiManagerParameter` for the broker address) or `initMQTT()`'s Wi-Fi logic is removed in favor of just setting the server.

### 5.4 `lib/sens` — Motion Detection

**`sens.h`**
```cpp
void initSens();
void updateSensStatus();
bool getJeepStatus();
```

**Behavior**
- Reads a digital vibration sensor on **GPIO 32**.
- Every time the pin reads `HIGH`, `lastSensTime` is reset to the current `millis()`.
- `jeepStatus` is `true` as long as fewer than `SENS_TIMEOUT` (60,000 ms / 1 minute) has elapsed since the last detected vibration — i.e., the vehicle is considered "running" for up to a minute after the last bump/vibration, then flips to idle.
- This debounced/latched status is used as a gate in `main.cpp` to avoid publishing telemetry (and unnecessary MQTT/network traffic) while the jeepney is parked or stationary.

---

## 6. System / Data Flow

```
      ┌────────────────┐        ┌──────────────────┐
      │ Vibration Sensor│──────▶│  sens.cpp          │
      │   (GPIO 32)     │       │  getJeepStatus()    │
      └────────────────┘        └─────────┬─────────┘
                                           │ (gates publishing)
      ┌────────────────┐        ┌─────────▼─────────┐        ┌───────────────┐
      │  GPS Module     │──────▶│  gps.cpp            │──────▶│   main.cpp     │
      │ (UART2, 9600bps)│       │  getGPSTelemetry()   │       │  loop()        │
      └────────────────┘        └──────────────────┘        └───────┬───────┘
                                                                      │ every 3s,
                                                                      │ if moving + valid fix
                                                              ┌───────▼───────┐
                                                              │  mqtt.cpp      │
                                                              │ publishTelemetry│
                                                              └───────┬───────┘
                                                                      │ MQTT (port 1883)
                                                                      ▼
                                                     topic: smartroute/vehicles/live
```

**Publish gating logic (in `loop()`):**

```
publish  ⇔  getJeepStatus() == true
          AND hasValidLocation() == true
          AND (millis() - lastPublishTime) >= 3000 ms
```

---

## 7. MQTT Interface

| Property | Value |
|---|---|
| Topic | `smartroute/vehicles/live` |
| QoS / retain | Default (`PubSubClient` default is QoS 0, not retained) |
| Payload format | JSON (manually constructed string) |
| Publish cadence | Every 3 seconds, only while vehicle is active with a valid GPS fix |
| Client ID | `SmartRoute-<random hex>` (regenerated on every reconnect attempt) |

**Example payload:**
```json
{"id":"jeep_default_01","lat":14.676000,"lng":121.043500,"spd":23.40}
```

---

## 8. Build & Flash Instructions

**Prerequisites:** [PlatformIO](https://platformio.org/) (CLI or VS Code extension).

```bash
# Clone the repository
git clone https://github.com/ALT-F4-sparkfest/ESP32-Firmware.git
cd ESP32-Firmware

# Build
pio run

# Flash to a connected ESP32 board
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor
```

On first boot (or after a Wi-Fi credentials reset), the ESP32 will broadcast an access point named **`SmartRoute_Setup`**. Connect to it with a phone or laptop, and the WiFiManager captive portal will prompt for:
- Wi-Fi SSID and password
- Vehicle ID (custom field, defaults to `jeep_default_01`)

---

## 9. Known Limitations / Observations

- **MQTT broker is never configured in the current `main.cpp` flow.** See §5.3 — `initMQTT()` (which sets the broker host) is unused after the switch to `WiFiManager`. This should be fixed before the device can publish telemetry in the field.
- **No TLS/authentication on MQTT** — connections are made over plaintext port 1883 with no username/password or certificate, which is a concern for a production fleet-tracking deployment.
- **No persistence of vehicle ID beyond WiFiManager's own storage conventions** — confirm behavior on power loss/reset if this matters operationally.
- **`test/` and `include/` directories are placeholders** (PlatformIO scaffolding) with no actual content yet — no automated tests currently exist.
- **Manual JSON construction** in `gps.cpp` is intentional (memory-conscious) but has no escaping — currently safe since `vehicle_id` is bounded to 20 chars and only user-set through the portal, but a malicious/malformed vehicle ID entry could produce invalid JSON.

---

## 10. Suggested Next Steps

1. Wire up MQTT broker configuration (host/port, and ideally credentials) through `WiFiManager`, mirroring how the Vehicle ID field was added.
2. Consider QoS 1 for telemetry publishing to reduce data loss on flaky vehicle Wi-Fi/cellular links.
3. Add basic input sanitization/escaping to `getGPSTelemetry()`.
4. Populate `test/` with unit tests for `sens.cpp`'s timeout logic and `gps.cpp`'s JSON formatting.
