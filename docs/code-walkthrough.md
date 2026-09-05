# Code Walkthrough — Line by Line

## Project Structure

```
led-multisensor/
├── platformio.ini              # Build config + libraries
├── include/
│   ├── config.h                # Pin definitions and constants
│   └── secrets.h               # WiFi/MQTT credentials (gitignored)
├── src/
│   ├── main.cpp                # Entry point — setup, loop, state machine
│   ├── inputs.h / .cpp         # Read potentiometer and button
│   ├── outputs.h / .cpp        # Control MOSFET (PWM), mode LED, light state
│   ├── radar.h / .cpp          # LD2410C radar communication and config
│   ├── wifi_manager.h / .cpp   # WiFi connect + auto-reconnect
│   ├── mqtt_handler.h / .cpp   # MQTT + Home Assistant auto-discovery
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
lib_deps = ncmreynolds/ld2410
```

This tells PlatformIO:
- Target the ESP32 chip (ESP-WROOM-32 DevKit)
- Use the Arduino framework (provides `digitalWrite`, `analogRead`, `analogWrite`, etc.)
- Serial monitor speed: 115200 baud (bits per second)
- Install the ld2410 library for radar communication

Equivalent in web terms: this is your `package.json` — it defines the build environment.

---

## `include/config.h` — Constants

```cpp
#define PIN_POTENTIOMETER  34
#define PIN_BUTTON         27
#define PIN_MOSFET         23
#define PIN_MODE_LED        2

#define PIN_RADAR_RX       16
#define PIN_RADAR_TX       17
#define RADAR_BAUD_RATE    256000  // LD2410C factory default
#define RADAR_MAX_GATE     8       // detect across full range (~6m)
#define RADAR_MOTION_SENSITIVITY     40  // movement detection (0-100, lower = more sensitive)
#define RADAR_STATIONARY_SENSITIVITY 10  // breathing/still presence (lower = more sensitive)
#define RADAR_IDLE_TIME    10      // seconds absent before "no one" reported
```

`#define` is C's way of creating named constants. At compile time, every `PIN_MOSFET` is replaced with `25`. This is like `const PIN_MOSFET = 25` in JavaScript, but happens at compile time (zero runtime cost).

**Why GPIO 23 for the MOSFET?** It's PWM-capable. On the ESP32, all digital pins can do PWM via LEDC channels, but GPIO 23 is a safe choice — no boot conflicts, no special functions.

**Why GPIO 16/17 for radar?** These are the default UART2 RX/TX pins on ESP32. Using hardware UART means no SoftwareSerial timing issues.

**Split sensitivity:** The LD2410C has separate thresholds for motion (Doppler shift from movement) and stationary presence (micro-movements like breathing). Motion produces strong radar returns — sensitivity 40 works well. Stationary presence is much weaker — sensitivity 10 is needed to detect it reliably.

```cpp
#define FADE_MAX_MS  500UL
```

Fade duration in milliseconds at full brightness (0→255). Duration scales with target brightness: `FADE_MAX_MS * targetBrightness / 255`. A fade to 50% brightness takes ~250ms, a fade to full takes 500ms. Minimum floor of 50ms prevents invisible short fades.

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

### Potentiometer

```cpp
int readPotentiometer() {
  return analogRead(PIN_POTENTIOMETER);
}
```

`analogRead()` is an Arduino built-in. It:
1. Activates the ADC (Analog-to-Digital Converter) on the pin
2. Samples the voltage (0–3.3V)
3. Converts it to a 10-bit number (0–1023)
4. Returns the result

This is like `fetch()` for voltage — it converts a physical phenomenon (voltage) into a number your code can use.

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

### PWM Brightness Control

```cpp
void setBrightness(uint8_t value) {
  analogWrite(PIN_MOSFET, value);
}
```

`analogWrite()` sends a PWM signal to the pin. On the ESP32, this internally uses the LEDC (LED Control) peripheral. The value (0–255) controls the duty cycle:
- `0` = 0% on, 100% off → LED off
- `127` = 50% on, 50% off → LED at half brightness
- `255` = 100% on, 0% off → LED at full brightness

The MOSFET switches the 24V circuit on and off ~490 times per second (the default PWM frequency). Your eyes perceive this as average brightness.

**Web analogy:** `analogWrite(127)` is like setting `opacity: 0.5` on an element — it's a continuous value, not just on/off.

### Mode LED

```cpp
void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
```

The built-in LED on GPIO 2 is active-high: `HIGH` = LED ON.

---

## `src/radar.cpp` — LD2410C Radar Communication

### Auto-Configuration

```cpp
static bool radarNeedsConfig() {
  if (radar.max_moving_gate != RADAR_MAX_GATE ||
      radar.max_stationary_gate != RADAR_MAX_GATE ||
      radar.sensor_idle_time != RADAR_IDLE_TIME) {
    return true;
  }
  for (uint8_t gate = 0; gate <= RADAR_MAX_GATE; gate++) {
    if (radar.motion_sensitivity[gate] != RADAR_MOTION_SENSITIVITY ||
        radar.stationary_sensitivity[gate] != RADAR_STATIONARY_SENSITIVITY) {
      return true;
    }
  }
  return false;
}
```

The LD2410C stores its configuration in flash memory. On first boot (or if you change constants), we write the config once. On subsequent boots, `radarNeedsConfig()` compares the sensor's current settings against our constants — if they match, we skip configuration. This gives us fast boot times and no unnecessary flash wear.

### Setup

```cpp
void setupRadar() {
  Serial2.setRxBufferSize(2048);
  Serial2.begin(RADAR_BAUD_RATE, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  delay(500);
  while (Serial2.available()) Serial2.read();

  sensorReady = radar.begin(Serial2, false);

  // Defensive: recover a sensor left stuck in config mode
  Serial2.write(CMD_LEAVE_CONFIG, sizeof(CMD_LEAVE_CONFIG));
  Serial2.flush();
  delay(100);
  while (Serial2.available()) Serial2.read();
  ...
}
```

Key points:
- **256000 baud** — the LD2410C's factory default. We never change it.
- **`Serial2.setRxBufferSize(2048)`** — larger buffer for reliable reception at high baud rates.
- **LEAVE_CFG command** — sent defensively at boot in case a previous power loss left the sensor stuck in config mode. Ignored when already in data mode.
- **`radar.begin(Serial2, false)`** — the `false` means "don't try to configure baud rate" (we know it's already 256000). This call always returns `true`, so we use the `sensorReady` flag only as a guard.

### Reading Presence

```cpp
bool radarPresenceDetected() {
  if (!sensorReady) return false;
  radar.read();
  return radar.presenceDetected();
}
```

`radar.read()` pulls available bytes from Serial2, parses complete frames, and updates the sensor's internal state. `presenceDetected()` returns `true` if the sensor sees a moving OR stationary target.

**Connection freshness:** `radar.isConnected()` checks whether a valid data frame was received within the last 3 seconds. If the sensor stops sending data (unplugged, baud mismatch, or firmware crash), it returns `false` and the web UI shows "Radar: OFFLINE".

The LD2410C uses two detection channels:
- **Moving targets** — detected via Doppler frequency shift
- **Stationary targets** — detected via micro-movements (breathing, slight posture changes)

Both count as "presence" for our purposes.

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
```

These persist across `loop()` calls (via `static`). They represent the complete state of the system:

| Variable | Type | Purpose |
|----------|------|---------|
| `currentMode` | Mode | PRESENCE or MANUAL |
| `presenceState` | PresenceState | IDLE or ACTIVE |
| `currentBrightness` | 0–255 | What the LED is currently at |
| `targetBrightness` | 0–255 | What we're fading toward |
| `lastPresence` | bool | Previous presence state (for serial output and web UI updates) |
| `fading` | bool | Whether a fade is in progress |

### Setup

```cpp
void setup() {
  Serial.begin(115200);
  setupInputs();
  setupOutputs();
  setupRadar();

  Serial.println(F("LED-on-presence started"));
  Serial.println(F("Mode: PRESENCE (default)"));
}
```

**`setup()` runs once** when the ESP32 powers on. Equivalent to a constructor or `useEffect([], ...)`.

**`F()` macro:** Wraps string literals to store them in flash memory instead of RAM. On the ESP32 this is less critical (520KB RAM vs Arduino's 2KB), but it's still good practice.

### The Loop

```cpp
void loop() {
  if (readButton()) {
    currentMode = (currentMode == MODE_PRESENCE) ? MODE_MANUAL : MODE_PRESENCE;
    Serial.print(F("Mode: "));
    Serial.println(currentMode == MODE_PRESENCE ? F("PRESENCE") : F("MANUAL"));
  }

  int potValue = readPotentiometer();
  bool presence = radarPresenceDetected();

  updatePresenceState(potValue, presence);

  if (lightOn) {
    fadeToward(map(potValue, 0, 1023, 255, 0));
  } else {
    fadeToward(0);
  }

  setBrightness(currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  printStatus(potValue, presence);
}
```

**Step-by-step:**

1. **Button check:** If pressed, toggle mode. The debounce logic is inside `readButton()`.

2. **Read sensors:** Get potentiometer position and radar presence state.

3. **Presence state machine:** `updatePresenceState()` handles the two states:
   - `PRESENCE_IDLE` → if presence detected, move to ACTIVE, turn light on
   - `PRESENCE_ACTIVE` → if presence goes away, move back to IDLE, turn light off
   - Manual mode → light on if pot > 5, ignore sensor

4. **Fade:** `fadeToward()` uses time-based fading with `FADE_MAX_MS`. It calculates the fraction of time elapsed since the fade started and interpolates brightness linearly from `fadeStartBrightness` to the target. Duration scales with target brightness: `FADE_MAX_MS * target / 255`.

5. **Output:** Write PWM value to MOSFET, update mode LED.

6. **Debug:** `printStatus()` prints sensor state every 500ms (not every loop, which would flood the serial monitor), including a loop counter.

**`map()` function:** Scales a number from one range to another. `map(potValue, 0, 1023, 255, 0)` converts the potentiometer's 0–1023 range to PWM's 255–0 range (inverted for clockwise = dimmer).

**`fadeToward()` helper:** Computes elapsed time since fade start, determines total fade duration based on target brightness, and linearly interpolates `currentBrightness`. `min()` and `max()` clamp the value so it doesn't overshoot. When the target is reached, `fading` is set to `false`.

---

## Debug Output

```cpp
static unsigned long lastPrint = 0;
if (millis() - lastPrint > 500) {
  lastPrint = millis();
  Serial.print(F("Pot: "));
  Serial.print(potValue);
  // ...
}
```

Prints debug info every 500ms (not every loop, which would flood the serial monitor). The `static` variable `lastPrint` tracks when we last printed.

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

## `src/mqtt_handler.cpp` — Home Assistant Integration

### Auto-Discovery

When MQTT connects, the device publishes JSON config messages to `homeassistant/` topics. Home Assistant sees these and automatically creates entities (light, sensors, binary sensors). No manual YAML config needed on the HA side.

### State Publishing

Every 2 seconds, the device publishes:
- Presence state (ON/OFF)
- Radar distance
- Light state (ON/OFF + brightness)
- Potentiometer value

### Command Subscription

Subscribes to `led-on-presence/config/mode/set` — when HA sends "PRESENCE" or "MANUAL", the device switches modes.

### Graceful Degradation

If MQTT broker is unreachable, the device continues without Home Assistant integration. All local functionality (radar, light, button, pot, web UI) works independently.

---

## `src/web_server.cpp` — Debug Web UI

Serves a minimal HTML page at `http://<esp32-ip>` with:
- Mode (PRESENCE or MANUAL)
- Radar connection status (ONLINE / OFFLINE)
- Motion state, Presence state, Distance
- Light state, Brightness, Potentiometer value
- Light toggle button
- Auto-refreshes every second

Also exposes `GET /api/status` (JSON) and `POST /api/toggle` for programmatic access.

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
| `setInterval(fn, 10)` | `loop()` (non-blocking) |
| CSS `opacity: 0.5` | `analogWrite(pin, 127)` |
| `element.style.opacity` | `analogWrite(pin, value)` |
| TypeScript type | `enum` / `#define` |
| `Math.min(a, b)` | `min(a, b)` |
| `value * 255 / 1023` | `map(value, 0, 1023, 0, 255)` |
| Node.js event loop | Arduino `loop()` (synchronous) |
