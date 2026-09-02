# Arduino Basics for Web Developers

## What is Arduino?

Arduino is a small, cheap computer designed for controlling electronics. Unlike your Mac, it has no screen, no keyboard, and no operating system. It runs one program (called a "sketch") in a never-ending loop.

Think of it as a very simple server that:
1. Runs `setup()` once at startup
2. Then runs `loop()` forever, ~10,000 times per second

```
Power On → setup() → loop() → loop() → loop() → ... (forever)
```

---

## Hardware: The Board

An Arduino Uno has:

| Feature | Spec | Web Equivalent |
|---------|------|----------------|
| CPU | ATmega328P, 16 MHz | Very slow laptop |
| RAM | 2 KB | A tiny `ArrayBuffer` |
| Flash | 32 KB | Your code storage (like disk) |
| Digital pins | 14 (D0–D13) | GPIO pins |
| Analog pins | 6 (A0–A5) | ADC pins (read voltages) |
| PWM pins | 6 (3,5,6,9,10,11) | DAC pins (variable output) |
| USB | Type B | Programming + power |
| Voltage | 5V logic | ⚠️ NOT 3.3V, NOT 12V, NOT 24V |

**Critical rule:** Never connect more than 5V to any Arduino pin. It will destroy the chip.

---

## Digital vs Analog Pins

### Digital Pins (D0–D13)

Read or write `HIGH` (5V) or `LOW` (0V). Binary — on or off.

```cpp
pinMode(13, OUTPUT);          // Configure pin 13 as output
digitalWrite(13, HIGH);       // Set pin 13 to 5V (LED on)
digitalWrite(13, LOW);        // Set pin 13 to 0V (LED off)
int val = digitalRead(2);     // Read pin 2: returns HIGH or LOW
```

**Web analogy:** `digitalWrite(pin, HIGH)` is like `element.classList.add('active')`.

### Analog Pins (A0–A5)

Read voltages 0–5V as numbers 0–1023 (10-bit ADC).

```cpp
int value = analogRead(A0);   // Returns 0–1023
```

**How ADC works:**
- 0V → returns 0
- 2.5V → returns ~512
- 5V → returns 1023

**Web analogy:** `analogRead(A0)` is like `event.clientX` — a continuous value, not binary.

### PWM Output (Special Digital Pins)

Some digital pins (3, 5, 6, 9, 10, 11) can output PWM — rapid on/off switching that simulates analog voltage:

```cpp
analogWrite(6, 0);     // 0% duty cycle → 0V average → LED off
analogWrite(6, 127);   // 50% duty cycle → ~2.5V average → half brightness
analogWrite(6, 255);   // 100% duty cycle → 5V average → full brightness
```

PWM is how you do dimming, motor speed control, etc. The Arduino switches the pin on and off ~490 times per second. Your eyes perceive the average as brightness.

**This project uses PWM** on pin 6 to control the IRLZ44N MOSFET, which switches the 24V LED strip.

**Note:** `analogWrite()` is NOT `analogRead()`. One outputs a signal, the other reads one.

---

## `pinMode()` — Setting Up Pins

Before using a pin, you must configure it:

```cpp
pinMode(pin, INPUT);        // Read a signal
pinMode(pin, INPUT_PULLUP); // Read with internal pull-up resistor
pinMode(pin, OUTPUT);       // Send a signal
```

**What's INPUT_PULLUP?**

Without a pull-up resistor, a disconnected pin "floats" — it reads random HIGH/LOW values. A pull-up resistor connects the pin to 5V through a large resistor (~20kΩ), so:
- Nothing connected → pin reads HIGH (pulled up)
- Connected to GND → pin reads LOW (overpowering the pull-up)

This is why our button uses `INPUT_PULLUP` — one leg to the pin, other leg to GND. Pressing the button connects the pin to GND, making it read LOW.

---

## Voltage Levels — The Most Important Concept

| Voltage | Arduino Pin | Meaning |
|---------|------------|---------|
| 0V | GND | Ground (reference point) |
| 0–1.5V | Digital | Reads as LOW |
| 1.5–5V | Digital | Reads as HIGH |
| 0–5V | Analog A0–A5 | Maps to 0–1023 |
| >5V | Any pin | **DAMAGE** — don't do this |

**Why the MOSFET matters:**

```
Arduino (5V world) ──→ MOSFET Gate ──→ MOSFET switches ──→ LED Strip (24V world)
```

The MOSFET is like a drawbridge between two countries with different rules. The Arduino sends a low-power 5V signal to the Gate. The MOSFET uses that signal to switch the high-power 24V circuit. The two sides share a common ground but the MOSFET controls the flow.

---

## `setup()` vs `loop()`

```cpp
void setup() {
  // Runs ONCE at startup
  // Configure pins, start serial, initialize
  pinMode(13, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // Runs forever, repeating
  // Main logic goes here
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  delay(1000);
}
```

**Web analogy:**
- `setup()` → `useEffect(() => { /* init */ }, [])` or a module's top-level code
- `loop()` → `setInterval(() => { /* repeated work */ }, 10)` or `requestAnimationFrame`

The loop doesn't "know" it's looping — each call is independent. You use global/static variables to remember state between iterations.

---

## Serial Communication — Your Debug Console

```cpp
Serial.begin(9600);              // Start communication
Serial.println("Hello");         // Print text + newline
Serial.println(42);              // Print number
Serial.println(variable);        // Print variable value
```

Open the serial monitor in PlatformIO:
```bash
pio device monitor
```

This shows what your Arduino is "saying." It's your `console.log()` — invaluable for debugging.

**Baud rate:** 9600 means 9600 bits per second. Both Arduino and monitor must use the same speed. 9600 is slow but reliable.

---

## `#define` vs `const` vs `static`

```cpp
#define PIN_BUTTON 3        // Preprocessor macro — text replacement at compile time
const int PIN_BUTTON = 3;   // Typed constant — compiler checks types
static int count = 0;       // Persistent variable — survives between function calls
```

| Feature | `#define` | `const` | `static` |
|---------|-----------|---------|----------|
| Scope | Global (file) | Current block | Current block |
| Type safety | No (raw text) | Yes | Yes |
| Memory | Flash (zero cost) | RAM or Flash | RAM |
| Persists between calls | N/A | No | Yes |

For pin definitions, `#define` is traditional in Arduino. For constants that need types, use `const`.

---

## Memory Management

Unlike JavaScript, there's no garbage collector. You manage memory manually:

- **Global variables:** Exist for the entire program lifetime. Use sparingly.
- **Local variables:** Created when a function is called, destroyed when it returns.
- **`static` variables:** Like local, but persist between calls. Use for state that a function needs to remember.
- **`F()` macro:** Stores string literals in flash (32 KB) instead of RAM (2 KB). Always use for `Serial.println()`.

**The Uno has 2 KB of RAM.** That's tiny. A single `String` object can eat 100+ bytes. Prefer `char[]` arrays and `F()` for strings.

---

## Common Gotchas for Web Devs

### 1. No `async` / `await`

Arduino is synchronous. `delay(1000)` blocks everything for 1 second. You cannot "await" a sensor reading.

**Solution:** Use `millis()` for non-blocking timing:
```cpp
unsigned long lastTime = 0;
void loop() {
  if (millis() - lastTime > 1000) {
    lastTime = millis();
    // Do something every second
  }
  // Other code runs immediately
}
```

### 2. No Dynamic Memory Allocation (Avoid It)

Don't use `new`, `malloc()`, or `String` in production Arduino code. The heap is tiny and will fragment.

### 3. No Error Handling

There are no try/catch blocks. If something fails (serial port, sensor), the code just continues or crashes. Debug with `Serial.println()`.

### 4. No Standard Library

There's no `npm install`. Libraries are installed via PlatformIO:
```bash
pio pkg install --library "library-name"
```
Or declared in `platformio.ini`:
```ini
lib_deps =
    arduino-libname/LibraryName
```

### 5. `int` is 16-bit

On Arduino Uno, `int` is 16 bits (-32768 to 32767). On your Mac, `int` is 32 bits. Use `long` (32-bit) or `unsigned long` (32-bit unsigned) for larger numbers.

### 6. `unsigned long` for Time

`millis()` returns `unsigned long`. When subtracting timestamps, use `unsigned long` variables:
```cpp
unsigned long now = millis();
unsigned long elapsed = now - lastTime;  // correct
```
If you use `int`, the subtraction can overflow and give wrong results.

---

## Uploading Code

### Build
```bash
pio run
```
Compiles your code. Like `npm run build`.

### Upload
```bash
pio run -t upload
```
Compiles + uploads to the Arduino via USB. Like `npm run deploy`.

### Monitor
```bash
pio device monitor
```
Shows serial output from the Arduino. Like opening Chrome DevTools console.

### All in One
```bash
pio run -t upload && pio device monitor
```

---

## What Happens When Power is Lost?

Arduino has no persistent storage for variables. When you unplug it:
- All variables reset to their initial values
- `setup()` runs again from scratch
- `loop()` starts over

This is like a server that restarts on every request — your state is gone unless you explicitly save it (e.g., to EEPROM).

---

## Resources

- [Arduino Language Reference](https://www.arduino.cc/reference/en/) — all functions documented
- [PlatformIO Documentation](https://docs.platformio.org/) — build system docs
- [Arduino Reddit](https://reddit.com/r/arduino) — community help
- [Arduino Starter Kit](https://store.arduino.cc/arduino-starter-kit) — if you want more components
