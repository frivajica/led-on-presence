# Code Walkthrough — Line by Line

## Project Structure

```
led-on-presence/
├── platformio.ini          # Build configuration (board, framework)
├── include/
│   └── config.h            # Pin definitions and constants
├── src/
│   ├── main.cpp            # Entry point — setup() and loop()
│   ├── inputs.h / .cpp     # Read sensors and buttons
│   ├── outputs.h / .cpp    # Control MOSFET and LED
│   └── radar.h / .cpp      # LD2410C radar communication
└── docs/                   # This documentation
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
#define PIN_MOSFET         25
#define PIN_MODE_LED        2

#define PIN_RADAR_RX       16
#define PIN_RADAR_TX       17
#define RADAR_BAUD_RATE    256000  // LD2410C factory default
#define RADAR_MAX_GATE     8       // detect across full range (~6m)
#define RADAR_GATE_SENSITIVITY 10  // per-gate energy threshold (lower = more sensitive)
#define RADAR_IDLE_TIME    10      // seconds absent before "no one" reported
```

`#define` is C's way of creating named constants. At compile time, every `PIN_MOSFET` is replaced with `25`. This is like `const PIN_MOSFET = 25` in JavaScript, but happens at compile time (zero runtime cost).

**Why GPIO 25 for the MOSFET?** It's PWM-capable. On the ESP32, all digital pins can do PWM via LEDC channels, but GPIO 25 is a safe choice — no boot conflicts, no special functions.

**Why GPIO 16/17 for radar?** These are the default UART2 RX/TX pins on ESP32. Using hardware UART means no SoftwareSerial timing issues.

```cpp
#define FADE_STEP  5
```

How many brightness levels to change per loop iteration. With a 20ms delay per loop, fading from 0 to 255 takes `255/5 × 0.02 = ~1 second`. Higher = faster fade.

```cpp
#define MOTION_TIMEOUT_MS 15000UL
```

How long (in milliseconds) the light stays on after the last detected motion. The `UL` suffix means "unsigned long" — required because 15000 doesn't fit in a 16-bit int on some platforms.

```cpp
enum Mode {
  MODE_MOTION,
  MODE_MANUAL
};
```

An enum is a type that can only be one of a set of values. Like a TypeScript union type: `type Mode = 'motion' | 'manual'`.

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
void setBrightness(uint8_t pin, uint8_t value) {
  analogWrite(pin, value);
}
```

`analogWrite()` sends a PWM signal to the pin. On the ESP32, this internally uses the LEDC (LED Control) peripheral. The value (0–255) controls the duty cycle:
- `0` = 0% on, 100% off → LED off
- `127` = 50% on, 50% off → LED at half brightness
- `255` = 100% on, 0% off → LED at full brightness

The MOSFET switches the 24V circuit on and off ~490 times per second (the default PWM frequency). Your eyes perceive this as average brightness.

**Web analogy:** `analogWrite(pin, 127)` is like setting `opacity: 0.5` on an element — it's a continuous value, not just on/off.

### Mode LED

```cpp
void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
```

The built-in LED on GPIO 2 is active-high: `HIGH` = LED ON.

---

## `src/main.cpp` — The State Machine

### State Variables

```cpp
static Mode currentMode = MODE_MOTION;
static MotionState motionState = MOTION_IDLE;
static uint8_t currentBrightness = 0;
static uint8_t targetBrightness = 0;
static bool lightOn = false;
static unsigned long lastMotionTime = 0;
static unsigned long cooldownStartTime = 0;
```

These persist across `loop()` calls (via `static`). They represent the complete state of the system:

| Variable | Type | Purpose |
|----------|------|---------|
| `currentMode` | Mode | MOTION or MANUAL |
| `motionState` | MotionState | IDLE, ACTIVE, or COOLDOWN |
| `currentBrightness` | 0–255 | What the LED is currently at |
| `targetBrightness` | 0–255 | What we're fading toward |
| `lightOn` | bool | Whether the light should be on at all |
| `lastMotionTime` | millis() | When motion was last detected |

### Setup

```cpp
void setup() {
  Serial.begin(9600);
  setupInputs();
  setupOutputs();
  setupRadar();

  Serial.println(F("LED-on-presence started"));
  Serial.println(F("Mode: MOTION (default)"));
}
```

**`setup()` runs once** when the ESP32 powers on. Equivalent to a constructor or `useEffect([], ...)`.

**`F()` macro:** Wraps string literals to store them in flash memory instead of RAM. On the ESP32 this is less critical (520KB RAM vs Arduino's 2KB), but it's still good practice.

### The Loop

```cpp
void loop() {
  // 1. Check button
  if (readButton()) {
    currentMode = (currentMode == MODE_MOTION) ? MODE_MANUAL : MODE_MOTION;
  }

  // 2. Read inputs
  int potValue = readPotentiometer();
  bool presence = radarPresenceDetected();

  // 3. Motion state machine
  if (currentMode == MODE_MOTION) {
    switch (motionState) {
      case MOTION_IDLE:
        if (presence) {
          motionState = MOTION_ACTIVE;
          lightOn = true;
          lastMotionTime = millis();
        }
        break;

      case MOTION_ACTIVE:
        if (presence) {
          lastMotionTime = millis();
        }
        if (millis() - lastMotionTime > MOTION_TIMEOUT_MS) {
          motionState = MOTION_COOLDOWN;
          cooldownStartTime = millis();
          lightOn = false;
        }
        break;

      case MOTION_COOLDOWN:
        if (millis() - cooldownStartTime > COOLDOWN_MS) {
          motionState = MOTION_IDLE;
        }
        break;
    }
  } else {
    // Manual mode: potentiometer directly controls on/off
    lightOn = potValue > 5;
    motionState = MOTION_IDLE;
  }

  // 4. Set target brightness
  if (lightOn) {
    targetBrightness = map(potValue, 0, 1023, 255, 0);
  } else {
    targetBrightness = 0;
  }

  // 5. Fade current toward target
  if (currentBrightness < targetBrightness) {
    currentBrightness = min((int)(currentBrightness + FADE_STEP), (int)targetBrightness);
  } else if (currentBrightness > targetBrightness) {
    currentBrightness = max((int)(currentBrightness - FADE_STEP), (int)targetBrightness);
  }

  // 6. Apply
  setBrightness(PIN_MOSFET, currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  delay(20);
}
```

**Step-by-step:**

1. **Button check:** If pressed, toggle mode. The debounce logic is inside `readButton()`.

2. **Read sensors:** Get potentiometer position and radar presence state.

3. **Motion state machine:** Three states:
   - `MOTION_IDLE` → if presence detected, move to ACTIVE, turn light on
   - `MOTION_ACTIVE` → keep resetting timer while presence detected, fade out after 15s timeout
   - `MOTION_COOLDOWN` → ignore sensor for 2s (prevents LED heat from re-triggering), then back to IDLE
   - Manual mode → light on if pot > 5, ignore sensor

4. **Target brightness:** Map potentiometer (0–1023) to PWM (0–255). Clockwise = dimmer (inverted map). If light is off, target is 0.

5. **Fade:** Gradually move `currentBrightness` toward `targetBrightness` by `FADE_STEP`. `min()` and `max()` prevent overshooting. The `(int)` casts are needed because ESP32's C++14 compiler is strict about mixed types in `min()`/`max()`.

6. **Output:** Write PWM value to MOSFET, update mode LED.

**`map()` function:** Scales a number from one range to another. `map(potValue, 0, 1023, 255, 0)` converts the potentiometer's 0–1023 range to PWM's 255–0 range (inverted for clockwise = dimmer).

**`min()` / `max()`:** Clamp the value so it doesn't exceed the target. Without these, `currentBrightness` could overshoot.

**`delay(20)`:** 20ms per loop = 50 loops per second. This controls the fade speed. With `FADE_STEP = 5`, the LED ramps from 0 to 255 in about 1 second.

**Why not use `delay(15000)` for the 15s timeout?** Because `delay()` blocks everything. While waiting, the ESP32 can't read the button or update the fade. The `millis()` approach is non-blocking — the loop keeps running, checking the time elapsed.

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
| `setInterval(fn, 10)` | `loop()` + `delay(20)` |
| CSS `opacity: 0.5` | `analogWrite(pin, 127)` |
| `element.style.opacity` | `analogWrite(pin, value)` |
| TypeScript type | `enum` / `#define` |
| `Math.min(a, b)` | `min(a, b)` |
| `value * 255 / 1023` | `map(value, 0, 1023, 0, 255)` |
| Node.js event loop | Arduino `loop()` (synchronous) |
