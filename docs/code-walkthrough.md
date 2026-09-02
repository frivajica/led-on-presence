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
│   └── outputs.h / .cpp    # Control MOSFET and LED
└── docs/                   # This documentation
```

---

## `platformio.ini` — Build Config

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 9600
```

This tells PlatformIO:
- Target the ATmega328P chip (Arduino Uno)
- Use the Arduino framework (provides `digitalWrite`, `analogRead`, `analogWrite`, etc.)
- Serial monitor speed: 9600 baud (bits per second)

Equivalent in web terms: this is your `package.json` — it defines the build environment.

---

## `include/config.h` — Constants

```cpp
#define PIN_POTENTIOMETER  A0
#define PIN_MOTION_SENSOR  2
#define PIN_BUTTON         3
#define PIN_MOSFET         6
#define PIN_MODE_LED       13
```

`#define` is C's way of creating named constants. At compile time, every `PIN_MOSFET` is replaced with `6`. This is like `const PIN_MOSFET = 6` in JavaScript, but happens at compile time (zero runtime cost).

**Why D6 for the MOSFET?** It's a PWM-capable pin. Not all digital pins can do PWM — only pins 3, 5, 6, 9, 10, and 11 on the Uno. If you connect the MOSFET to a non-PWM pin, `analogWrite()` won't work.

```cpp
#define FADE_STEP  5
```

How many brightness levels to change per loop iteration. With a 20ms delay per loop, fading from 0 to 255 takes `255/5 × 0.02 = ~1 second`. Higher = faster fade.

```cpp
#define MOTION_TIMEOUT_MS 30000UL
```

How long (in milliseconds) the light stays on after the last detected motion. The `UL` suffix means "unsigned long" — required because 30000 doesn't fit in a 16-bit int.

```cpp
enum Mode {
  MODE_MOTION,
  MODE_MANUAL
};
```

An enum is a type that can only be one of a set of values. Like a TypeScript union type: `type Mode = 'motion' | 'manual'`.

---

## `src/inputs.cpp` — Reading Inputs

### Potentiometer

```cpp
int readPotentiometer() {
  return analogRead(PIN_POTENTIOMETER);
}
```

`analogRead()` is an Arduino built-in. It:
1. Activates the ADC (Analog-to-Digital Converter) on the pin
2. Samples the voltage (0–5V)
3. Converts it to a 10-bit number (0–1023)
4. Returns the result

This is like `fetch()` for voltage — it converts a physical phenomenon (voltage) into a number your code can use.

### Motion Sensor

```cpp
bool readMotion() {
  return digitalRead(PIN_MOTION_SENSOR) == HIGH;
}
```

`digitalRead()` reads a digital pin. It returns `HIGH` (≥3V) or `LOW` (<3V). The PIR sensor outputs 3.3V when motion is detected, which Arduino reads as `HIGH`.

### Button with Debounce

```cpp
static bool lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0;

bool readButton() {
  bool reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  lastButtonState = reading;

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    return reading == LOW;
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

**`millis()`:** Returns milliseconds since the Arduino was powered on. Like `Date.now()` in JS, but starts from 0 when the board resets.

---

## `src/outputs.cpp` — Controlling Outputs

### PWM Brightness Control

```cpp
void setBrightness(uint8_t pin, uint8_t value) {
  analogWrite(pin, value);
}
```

`analogWrite()` sends a PWM signal to the pin. The value (0–255) controls the duty cycle:
- `0` = 0% on, 100% off → LED off
- `127` = 50% on, 50% off → LED at half brightness
- `255` = 100% on, 0% off → LED at full brightness

The MOSFET switches the 24V circuit on and off ~490 times per second (the Arduino's PWM frequency). Your eyes perceive this as average brightness.

**Web analogy:** `analogWrite(pin, 127)` is like setting `opacity: 0.5` on an element — it's a continuous value, not just on/off.

### Mode LED

```cpp
void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
```

The built-in LED on pin 13 is active-high: `HIGH` = LED ON.

---

## `src/main.cpp` — The State Machine

### State Variables

```cpp
static Mode currentMode = MODE_MOTION;
static uint8_t currentBrightness = 0;
static uint8_t targetBrightness = 0;
static bool lightOn = false;
static unsigned long lastMotionTime = 0;
```

These persist across `loop()` calls (via `static`). They represent the complete state of the system:

| Variable | Type | Purpose |
|----------|------|---------|
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

  Serial.println(F("LED-on-presence started"));
  Serial.println(F("Mode: MOTION (default)"));
}
```

**`setup()` runs once** when the Arduino powers on. Equivalent to a constructor or `useEffect([], ...)`.

**`F()` macro:** Wraps string literals to store them in flash memory instead of RAM. The Uno only has 2 KB of RAM, so this is important. Without `F()`, every string literal would be copied into RAM at startup.

### The Loop

```cpp
void loop() {
  // 1. Check button
  if (readButton()) {
    currentMode = (currentMode == MODE_MOTION) ? MODE_MANUAL : MODE_MOTION;
  }

  // 2. Read inputs
  int potValue = readPotentiometer();
  bool motionDetected = readMotion();

  // 3. Determine if light should be on
  if (currentMode == MODE_MOTION) {
    if (motionDetected) {
      lightOn = true;
      lastMotionTime = millis();
    } else if (millis() - lastMotionTime > MOTION_TIMEOUT_MS) {
      lightOn = false;
    }
  } else {
    lightOn = potValue > 0;
  }

  // 4. Set target brightness
  if (lightOn) {
    targetBrightness = map(potValue, 0, 1023, 0, 255);
  } else {
    targetBrightness = 0;
  }

  // 5. Fade current toward target
  if (currentBrightness < targetBrightness) {
    currentBrightness = min(currentBrightness + FADE_STEP, targetBrightness);
  } else if (currentBrightness > targetBrightness) {
    currentBrightness = max(currentBrightness - FADE_STEP, targetBrightness);
  }

  // 6. Apply
  setBrightness(PIN_MOSFET, currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  delay(20);
}
```

**Step-by-step:**

1. **Button check:** If pressed, toggle mode. The debounce logic is inside `readButton()`.

2. **Read sensors:** Get potentiometer position and motion state.

3. **Motion logic:**
   - Motion mode + motion detected → turn light on, record timestamp
   - Motion mode + no motion for 30s → turn light off
   - Manual mode → light on if pot > 0

4. **Target brightness:** Map potentiometer (0–1023) to PWM (0–255). If light is off, target is 0.

5. **Fade:** Gradually move `currentBrightness` toward `targetBrightness` by `FADE_STEP`. `min()` and `max()` prevent overshooting.

6. **Output:** Write PWM value to MOSFET, update mode LED.

**`map()` function:** Scales a number from one range to another. `map(potValue, 0, 1023, 0, 255)` converts the potentiometer's 0–1023 range to PWM's 0–255 range.

**`min()` / `max()`:** Clamp the value so it doesn't exceed the target. Without these, `currentBrightness` could overshoot.

**`delay(20)`:** 20ms per loop = 50 loops per second. This controls the fade speed. With `FADE_STEP = 5`, the LED ramps from 0 to 255 in about 1 second.

**Why not use `delay(1000)` for the 30s timeout?** Because `delay()` blocks everything. While waiting, the Arduino can't read the button or update the fade. The `millis()` approach is non-blocking — the loop keeps running, checking the time elapsed.

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
