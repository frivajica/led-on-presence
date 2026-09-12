# Code Walkthrough — Line by Line

## Project Structure

```
led-on-presence/
├── platformio.ini              # Build config + libraries
├── include/
│   ├── config.h                # Pin definitions and constants
│   └── secrets.h               # WiFi credentials (gitignored)
├── src/
│   ├── main.cpp                # Entry point — setup, loop, state machine
│   ├── inputs.h / .cpp         # Read potentiometer (EMA-filtered) and button
│   ├── outputs.h / .cpp        # Control MOSFET (SigmaDelta PWM), mode LED
│   ├── radar.h / .cpp          # LD2410C radar (autoReadTask, EMI filter)
│   ├── wifi_manager.h / .cpp   # WiFi connect + auto-reconnect
│   └── web_server.h / .cpp     # Minimal web UI for debugging
└── docs/                       # This documentation
```

---

## `platformio.ini` — Build Config

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_protocol = espota
upload_port = 192.168.1.203
lib_deps =
    ncmreynolds/ld2410
    bblanchon/ArduinoJson
    esp32async/ESPAsyncWebServer
    esp32async/AsyncTCP
```

This tells PlatformIO:
- Target the ESP32 chip (ESP-WROOM-32 DevKit)
- Use the Arduino framework
- Serial monitor speed: 115200 baud
- Upload via OTA (WiFi) to the configured IP
- Install required libraries

---

## `include/config.h` — Constants

```cpp
#define PIN_POTENTIOMETER  34
#define PIN_BUTTON         27
#define PIN_MOSFET         25
#define PIN_MODE_LED        2

#define PIN_RADAR_RX       16
#define PIN_RADAR_TX       17
#define RADAR_BAUD_RATE    115200  // Configured via Bluetooth app
```

`#define` is C's way of creating named constants. At compile time, every `PIN_MOSFET` is replaced with `25`. This is like `const PIN_MOSFET = 25` in JavaScript, but happens at compile time (zero runtime cost).

**Why GPIO 25 for the MOSFET?** It's PWM-capable with no boot conflicts or special functions.

**Why GPIO 16/17 for radar?** These are the default UART2 RX/TX pins on ESP32. Using hardware UART means no SoftwareSerial timing issues.

```cpp
#define DEBOUNCE_MS        50   // Button debounce delay in milliseconds
#define PRESENCE_DEBOUNCE_MS  100  // Bidirectional: filter EMI on detect and loss (ms)
#define PRESENCE_COUNTDOWN_MS 15000 // Countdown before light turns off (ms)
```

**Presence debounce (100ms):** Applied in both directions. When the radar first reports presence, a 100ms timer starts — only if presence persists does it activate. When presence is lost, the same 100ms debounce fires before the 15-second countdown begins.

**Software countdown (15s):** After the radar reports absence + 100ms debounce, a 15-second countdown begins. The light stays on during this entire period. This prevents brief radar dropouts from turning off the light.

```cpp
#define BRIGHTNESS_HYSTERESIS 3    // Min change to update PWM — filters ADC noise
#define FADE_MAX_MS         500UL  // Fade duration at full brightness (0-255). Scales with target.
#define MAX_BRIGHTNESS      255    // 8-bit SigmaDelta
#define PWM_FREQUENCY       2000000 // 2 MHz SigmaDelta carrier frequency
```

**PWM hysteresis (±3):** The brightness target only updates when the change from the current target exceeds 3 units (out of 255). This prevents ADC noise from causing constant micro-updates to the PWM output.

**SigmaDelta at 2 MHz:** Unlike LEDC's square-wave PWM, SigmaDelta uses noise shaping at 2 MHz. The MOSFET's gate capacitance + 220Ω series resistor naturally low-pass this into a smooth analog DC voltage, preventing the MOSFET from oscillating in its linear region.

```cpp
enum Mode {
  MODE_PRESENCE,
  MODE_MANUAL
};
```

An enum is a type that can only be one of a set of values. Like a TypeScript union type: `type Mode = 'presence' | 'manual'`.

---

## `src/inputs.cpp` — Reading Inputs

### ADC Resolution

```cpp
void setupInputs() {
  analogReadResolution(10);
  pinMode(PIN_POTENTIOMETER, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}
```

The ESP32 defaults to 12-bit ADC (0–4095). We set it to 10-bit (0–1023) to match the Arduino Uno's range. This means the `map()` call in `main.cpp` works without changes.

### Potentiometer with Dual Filtering

```cpp
static float emaValue = -1;
static constexpr float EMA_ALPHA = 0.15;

int readPotentiometer() {
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) {
    sum += analogRead(PIN_POTENTIOMETER);
  }
  int sample = sum / 8;

  if (emaValue < 0) {
    emaValue = sample;
  } else {
    emaValue = EMA_ALPHA * sample + (1.0 - EMA_ALPHA) * emaValue;
  }

  lastPotValue = (int)emaValue;
  return lastPotValue;
}
```

**Two layers of noise reduction:**

1. **8-sample averaging:** Each call takes 8 rapid ADC reads and averages them. Each read takes ~10 µs, so 8 reads take < 100 µs — no perceptible delay. This reduces the ESP32's inherent ADC thermal noise (~50–100 LSB jitter).

2. **Exponential Moving Average (EMA):** The averaged sample is fed into an EMA with α=0.15. This gives a smoothing effect equivalent to ~13 samples over time:
   ```
   emaValue = 0.15 * currentSample + 0.85 * previousEmaValue
   ```
   This rejects slow power rail noise from the 24V→5V buck converter that would otherwise cause the brightness to fluctuate.

### Button with Debounce

```cpp
static bool lastButtonState = HIGH;
static bool lastStableState = HIGH;
static unsigned long lastDebounceTime = 0;

bool readButton() {
  bool reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  lastButtonState = reading;

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != lastStableState) {
      lastStableState = reading;
      return reading == LOW;
    }
  }

  return false;
}
```

**Why debounce?** Mechanical buttons "bounce" — when pressed, the metal contacts vibrate and create rapid HIGH/LOW transitions for a few milliseconds. Without debouncing, one press might register as 5–10 rapid presses.

**How it works:**
1. Read the pin state
2. If it changed from last time, record the timestamp (`millis()`)
3. If it's been stable for 50ms (`DEBOUNCE_MS`), accept the state
4. Only return `true` (pressed) when the state is `LOW` (active-low with pull-up)

**`static` variables:** These persist between function calls (like closure state in JavaScript). `lastButtonState` remembers its value from the previous call.

**`millis()`:** Returns milliseconds since the ESP32 was powered on. Like `Date.now()` in JS, but starts from 0 when the board resets.

---

## `src/outputs.cpp` — Controlling Outputs

### SigmaDelta PWM Brightness Control

```cpp
void setupOutputs() {
  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(PIN_MODE_LED, OUTPUT);

  sigmaDeltaAttach(PIN_MOSFET, PWM_FREQUENCY);
  sigmaDeltaWrite(PIN_MOSFET, 0);

  digitalWrite(PIN_MODE_LED, LOW);
}

void setBrightness(uint8_t value) {
  if (value == lastBrightness) return;
  sigmaDeltaWrite(PIN_MOSFET, value);
  lastBrightness = value;
}
```

**SigmaDelta vs LEDC:** The ESP32's SigmaDelta peripheral uses noise shaping at 2 MHz instead of fixed-width square waves. At this frequency, the MOSFET's own gate capacitance (~1200 pF) combined with the 220Ω series resistor acts as an RC low-pass filter, converting the high-frequency noise-shaped signal into a smooth DC voltage at the gate. This prevents the fast micro-stutter caused by the MOSFET operating at the edge of its linear region with only 3.3V drive.

**Write guard:** `setBrightness()` returns early if the value hasn't changed. This prevents hammering `sigmaDeltaWrite()` every loop iteration (thousands of times/sec), which could cause subtle PWM jitter.

### Mode LED

```cpp
void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
```

The built-in LED on GPIO 2 is active-high: `HIGH` = LED ON.

---

## `src/radar.cpp` — LD2410C Radar Communication

### Setup

```cpp
void setupRadar() {
  Serial2.setRxBufferSize(2048);
  Serial2.begin(RADAR_BAUD_RATE, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  delay(500);
  while (Serial2.available()) Serial2.read();

  sensorReady = radar.begin(Serial2, true);

  if (!sensorReady) {
    Serial.println(F("Radar: no response to firmware query — check wiring/baud rate"));
    radar.autoReadTask();
    return;
  }

  Serial.print(F("Radar: firmware v"));
  // ... print version info

  radar.autoReadTask();
}
```

Key points:
- **115200 baud** — configured via the HLKRadarTool Bluetooth app (not factory default of 256000)
- **`Serial2.setRxBufferSize(2048)`** — larger buffer for reliable reception
- **`radar.begin(Serial2, true)`** — the `true` means "wait for response and verify connection"
- **`radar.autoReadTask()`** — starts a FreeRTOS background task that continuously reads UART frames. This runs independently of the main loop, so no blocking UART reads are needed.

### Reading Presence with EMI Filter

```cpp
bool radarPresenceDetected() {
  if (!sensorReady || !radar.isConnected()) return false;
  if (!radar.presenceDetected()) return false;
  // EMI noise can report presence with 0 distance — filter it out
  return radar.detectionDistance() > 0;
}
```

**The EMI filter:** The buck converter's switching noise corrupts UART frames into ghost presence reports. These corrupted frames always report `distance = 0cm` (physically impossible — the LD2410C's minimum detection range is ~75cm). The filter rejects these by requiring `detectionDistance() > 0`.

**Zero delay on real targets:** A real person always reports a valid distance. The filter adds zero latency — it only blocks impossible reports.

**How `autoReadTask()` works:** It's an ESP32 FreeRTOS background task that:
1. Drains available bytes from the UART into a circular buffer
2. Parses complete data frames
3. Updates the sensor's internal state (target type, distance, energy levels)
4. Sleeps 10ms between iterations

The main loop reads this state directly — no blocking `radar.read()` calls needed.

---

## `src/main.cpp` — The State Machine

### State Variables

```cpp
static Mode currentMode = MODE_PRESENCE;
static PresenceState presenceState = PRESENCE_IDLE;
static uint8_t currentBrightness = 0;
static uint8_t targetBrightness = 0;
static bool lastPresence = false;

static uint8_t fadeStartBrightness = 0;
static unsigned long fadeStartTime = 0;
static bool fading = false;

static int stablePotValue = 0;

static unsigned long pendingSince = 0;
static unsigned long countdownStart = 0;
static bool pendingPresenceLost = false;
static bool countdownActive = false;
static bool countdownCompleted = false;
static bool effectivePresence = true;
static bool pendingPresenceDetected = false;
static unsigned long pendingDetectSince = 0;
```

| Variable | Type | Purpose |
|----------|------|---------|
| `currentMode` | Mode | PRESENCE or MANUAL |
| `presenceState` | PresenceState | IDLE or ACTIVE |
| `currentBrightness` | 0–255 | What the LED is currently at |
| `targetBrightness` | 0–255 | What we're fading toward |
| `lastPresence` | bool | Previous presence state (for fade trigger) |
| `effectivePresence` | bool | The actual presence state after debounce + countdown |
| `countdownActive` | bool | 15-second countdown is running |
| `countdownCompleted` | bool | Countdown finished (light ready to turn off) |
| `pendingPresenceLost` | bool | Radar just went absent — debouncing |
| `pendingPresenceDetected` | bool | Radar just appeared — debouncing |

### The Loop

```cpp
void loop() {
  int potValue = readPotentiometer();
  bool radarPresence = radarPresenceDetected();

  // Stable pot value filter
  if (abs(potValue - stablePotValue) > 2) {
    stablePotValue = potValue;
  }

  // Presence debounce + countdown state machine
  if (radarPresence) {
    // Reset loss state, start detection debounce if needed
    pendingPresenceLost = false;
    countdownActive = false;
    countdownStart = 0;
    countdownCompleted = false;

    if (!effectivePresence) {
      if (!pendingPresenceDetected) {
        pendingPresenceDetected = true;
        pendingDetectSince = millis();
      } else if (millis() - pendingDetectSince >= PRESENCE_DEBOUNCE_MS) {
        effectivePresence = true;
      }
    }
  } else {
    // Reset detection state, start loss debounce
    pendingPresenceDetected = false;

    if (!pendingPresenceLost) {
      pendingPresenceLost = true;
      pendingSince = millis();
      countdownCompleted = false;
      effectivePresence = true;
    } else if (millis() - pendingSince >= PRESENCE_DEBOUNCE_MS) {
      if (!countdownActive && !countdownCompleted) {
        countdownActive = true;
        countdownStart = millis();
      }
      unsigned long elapsed = millis() - countdownStart;
      if (elapsed >= PRESENCE_COUNTDOWN_MS) {
        effectivePresence = false;
        countdownActive = false;
        countdownCompleted = true;
      } else {
        effectivePresence = true;  // Light stays on during countdown
      }
    } else {
      effectivePresence = true;  // Within 100ms debounce window
    }
  }

  updatePresenceState(stablePotValue, effectivePresence);

  int target = isLightOn() ? map(stablePotValue, 0, 1023, 255, 0) : 0;
  if (target > MAX_BRIGHTNESS) target = MAX_BRIGHTNESS;

  // Fade logic — only start fade on direction change
  if (effectivePresence != lastPresence) {
    bool directionChanged = (effectivePresence && targetBrightness == 0) ||
                            (!effectivePresence && targetBrightness > 0);
    if (directionChanged) {
      fadeStart(target);
    }
  } else if (abs((int)target - (int)targetBrightness) >= BRIGHTNESS_HYSTERESIS) {
    if (fading) {
      targetBrightness = target;
    } else {
      currentBrightness = target;
      targetBrightness = target;
    }
  }

  fadeUpdate();
  lastPresence = effectivePresence;

  setBrightness(currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  if (readButton()) {
    currentMode = (currentMode == MODE_PRESENCE) ? MODE_MANUAL : MODE_PRESENCE;
  }

  ArduinoOTA.handle();
  wifiLoop();

  printStatus(stablePotValue, radarPresence, radarConnected, effectivePresence, target);
}
```

**Step-by-step:**

1. **Read sensors:** Get filtered potentiometer value and radar presence (after EMI filter).

2. **Stable pot filter:** Only update `stablePotValue` when the raw reading differs by > 2. Combined with the 8-sample averaging and EMA in `readPotentiometer()`, this creates triple noise filtering.

3. **Presence debounce + countdown:**
   - **Radar reports presence:** Start 100ms detection debounce. If presence persists → activate. Cancel any countdown.
   - **Radar reports absence:** Start 100ms loss debounce. If absence persists → begin 15s countdown. Light stays on during countdown.
   - **Countdown complete:** `effectivePresence = false`, light fades off.

4. **Brightness mapping:** Convert pot value (0–1023) to PWM target (255–0, inverted for clockwise = dimmer). Cap at `MAX_BRIGHTNESS`.

5. **Hysteresis check:** Only update target when the change exceeds `BRIGHTNESS_HYSTERESIS` (3 units). This prevents ADC noise from causing constant micro-updates.

6. **Fade:** Time-based linear interpolation from `fadeStartBrightness` to target. Duration scales with brightness: `500ms * target / 255`. Minimum 50ms floor prevents invisible short fades.

7. **Output:** Write PWM value (with write guard — only if changed), update mode LED.

8. **Background:** Handle OTA updates, WiFi reconnection, and serial debug output.

### Fade Logic

```cpp
static void fadeUpdate() {
  if (!fading) return;
  unsigned long duration = (unsigned long)FADE_MAX_MS * targetBrightness / 255;
  if (duration < 50) duration = 50;
  unsigned long elapsed = millis() - fadeStartTime;
  if (elapsed >= duration) {
    currentBrightness = targetBrightness;
    fading = false;
  } else {
    float progress = (float)elapsed / duration;
    currentBrightness = fadeStartBrightness +
        (int)((int)targetBrightness - (int)fadeStartBrightness) * progress;
  }
}
```

Duration scales with target brightness: a fade to 50% takes ~250ms, to full takes 500ms. The 50ms minimum prevents fades that are too short to see.

---

## Debug Output

```cpp
static void printStatus(int potValue, bool radarPresence, bool radarConnected, bool effectivePresence, int target) {
  // Prints every 500ms (not every loop)
  // Includes loop counter, WiFi state
}
```

Sample output:
```
Loop: 2382/s WiFi: OK
 Pot: 439 Bright: 146/146 Radar: Y RCon: Y Eff: Y Countdown: - State: ACTIVE Light: ON Tgt: 146 Dist: 150cm Fade: N
```

`RADAR OFFLINE` / `RADAR ONLINE` events are printed with timestamps when the UART connection state changes. These help diagnose EMI from the buck converter.

---

## `src/wifi_manager.cpp` — WiFi Connection

```cpp
void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // ... waits up to 10s for connection
}

void wifiLoop() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastReconnectAttempt > 5000) {
    WiFi.reconnect();
  }
}
```

Non-blocking WiFi with 10-second timeout on boot. If WiFi fails, the device continues working locally (radar, light, button, pot all work offline). Reconnection attempts every 5 seconds in the background.

---

## `src/web_server.cpp` — Debug Web UI

Serves a minimal HTML page at `http://<esp32-ip>` with:
- Mode (PRESENCE or MANUAL)
- Radar connection status (ONLINE / OFFLINE)
- Motion state, Presence state, Distance
- Light state, Brightness, Potentiometer value
- Auto-refreshes every second via `fetch('/api/status')`

Also exposes `GET /api/status` (JSON) for programmatic access.

---

## Key Differences from Web Development

| Web Dev Concept | Arduino Equivalent |
|-----------------|-------------------|
| `package.json` | `platformio.ini` |
| `npm run build` | `pio run` |
| `npm start` | `pio run -t upload` |
| `console.log()` | `Serial.println()` |
| `Date.now()` | `millis()` |
| `const` / `let` | `#define` / `static` variables |
| `useEffect(fn, [])` | `setup()` |
| `setInterval(fn, 10)` | `loop()` (synchronous, non-blocking) |
| CSS `opacity: 0.5` | `sigmaDeltaWrite(pin, 127)` |
| TypeScript type | `enum` / `#define` |
| `Math.min(a, b)` | `min(a, b)` |
| `value * 255 / 1023` | `map(value, 0, 1023, 0, 255)` |
| Node.js event loop | Arduino `loop()` (synchronous) |
| FreeRTOS task | `autoReadTask()` (background UART reader) |
