# Arduino Basics for Web Developers

## What is Arduino?

Arduino is a small, cheap computer designed for controlling electronics. Unlike your Mac, it has no screen, no keyboard, and no operating system. It runs one program (called a "sketch") in a never-ending loop.

This project uses an ESP32 (ESP-WROOM-32 DevKit), which is Arduino-compatible — it uses the same programming language and tools, but is much more powerful.

Think of it as a very simple server that:
1. Runs `setup()` once at startup
2. Then runs `loop()` forever, ~10,000 times per second

```
Power On → setup() → loop() → loop() → loop() → ... (forever)
```

---

## Hardware: The Board

An ESP32 DevKit V1 has:

| Feature | Spec | Web Equivalent |
|---------|------|----------------|
| CPU | Dual-core Xtensa, 240 MHz | Slow laptop |
| RAM | 520 KB | A small `ArrayBuffer` |
| Flash | 4 MB | Your code storage (like disk) |
| Digital pins | 34 (GPIO 0–39) | GPIO pins |
| Analog pins | 18 (GPIO 0–39) | ADC pins (read voltages) |
| PWM | All digital pins (via LEDC) | DAC pins (variable output) |
| UART | 3 hardware serial ports | Serial interfaces |
| USB | Type-C or Micro-USB | Programming + power |
| Voltage | 3.3V logic | ⚠️ NOT 5V, NOT 12V, NOT 24V |

**Critical rule:** Never connect more than 3.3V to any ESP32 GPIO pin. It will destroy the chip.

**Difference from Arduino Uno:** The Uno uses 5V logic. The ESP32 uses 3.3V. This is actually better for this project because the LD2410C radar is also 3.3V — no voltage conversion needed.

---

## Digital vs Analog Pins

### Digital Pins (GPIO 0–39)

Read or write `HIGH` (3.3V) or `LOW` (0V). Binary — on or off.

```cpp
 pinMode(2, OUTPUT);           // Configure GPIO 2 as output
 digitalWrite(2, HIGH);        // Set GPIO 2 to 3.3V (LED on)
 digitalWrite(2, LOW);         // Set GPIO 2 to 0V (LED off)
 int val = digitalRead(27);    // Read GPIO 27: returns HIGH or LOW
```

**Web analogy:** `digitalWrite(pin, HIGH)` is like `element.classList.add('active')`.

**ESP32 note:** GPIO 6–11 are reserved for flash memory — never use these pins. GPIO 34–39 are input-only (can read but not write).

### Analog Pins (GPIO 0–39)

Read voltages 0–3.3V as numbers 0–4095 (12-bit ADC by default).

```cpp
int value = analogRead(34);    // Returns 0–4095
```

**How ADC works:**
- 0V → returns 0
- 1.65V → returns ~2048
- 3.3V → returns 4095

**Web analogy:** `analogRead(34)` is like `event.clientX` — a continuous value, not binary.

**Note:** This project uses `analogReadResolution(10)` to match the Arduino Uno's 0–1023 range.

### PWM Output (All Digital Pins)

On the ESP32, all digital pins can output PWM via LEDC (LED Control) channels:

```cpp
analogWrite(25, 0);     // 0% duty cycle → 0V average → LED off
analogWrite(25, 127);   // 50% duty cycle → ~1.65V average → half brightness
analogWrite(25, 255);   // 100% duty cycle → 3.3V average → full brightness
```

PWM is how you do dimming, motor speed control, etc. The ESP32 switches the pin on and off ~490 times per second. Your eyes perceive the average as brightness.

**This project uses PWM** on GPIO 25 to control the IRLZ44N MOSFET, which switches the 24V LED strip.

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

Without a pull-up resistor, a disconnected pin "floats" — it reads random HIGH/LOW values. A pull-up resistor connects the pin to 3.3V through a large resistor (~45kΩ on ESP32), so:
- Nothing connected → pin reads HIGH (pulled up)
- Connected to GND → pin reads LOW (overpowering the pull-up)

This is why our button uses `INPUT_PULLUP` — one leg to the pin, other leg to GND. Pressing the button connects the pin to GND, making it read LOW.

---

## Voltage Levels — The Most Important Concept

| Voltage | ESP32 Pin | Meaning |
|---------|----------|---------|
| 0V | GND | Ground (reference point) |
| 0–1.0V | Digital | Reads as LOW |
| 1.0–3.3V | Digital | Reads as HIGH |
| 0–3.3V | Analog GPIO | Maps to 0–4095 |
| >3.3V | Any GPIO pin | **DAMAGE** — don't do this |

**Why the MOSFET matters:**

```
ESP32 (3.3V world) ──→ MOSFET Gate ──→ MOSFET switches ──→ LED Strip (24V world)
```

The MOSFET is like a drawbridge between two countries with different rules. The ESP32 sends a low-power 3.3V signal to the Gate. The MOSFET uses that signal to switch the high-power 24V circuit. The two sides share a common ground but the MOSFET controls the flow.

---

## `setup()` vs `loop()`

```cpp
void setup() {
  // Runs ONCE at startup
  // Configure pins, start serial, initialize
  pinMode(2, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // Runs forever, repeating
  // Main logic goes here
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
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
Serial.begin(115200);            // Start communication
Serial.println("Hello");         // Print text + newline
Serial.println(42);              // Print number
Serial.println(variable);        // Print variable value
```

Open the serial monitor in PlatformIO:
```bash
pio device monitor
```

This shows what your ESP32 is "saying." It's your `console.log()` — invaluable for debugging.

**Baud rate:** 115200 means 115200 bits per second. Both ESP32 and monitor must use the same speed. 115200 is faster and still reliable.

---

## `#define` vs `const` vs `static`

```cpp
#define PIN_BUTTON 27       // Preprocessor macro — text replacement at compile time
const int PIN_BUTTON = 27;  // Typed constant — compiler checks types
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
- **`F()` macro:** Stores string literals in flash instead of RAM. Good practice even on ESP32 (which has plenty of RAM).

**The ESP32 has 520 KB of RAM.** That's plenty for this project. No need to worry about memory like you would on an Arduino Uno (2KB).

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

Don't use `new`, `malloc()`, or `String` in production Arduino code. The heap can fragment.

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

### 5. `int` is 32-bit on ESP32

On ESP32, `int` is 32 bits (same as your Mac). On Arduino Uno, `int` is only 16 bits. This project works on both, but it's good to know the difference.

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
Compiles + uploads to the ESP32 via USB. Like `npm run deploy`.

### Monitor
```bash
pio device monitor
```
Shows serial output from the ESP32. Like opening Chrome DevTools console.

### All in One
```bash
pio run -t upload && pio device monitor
```

---

## What Happens When Power is Lost?

ESP32 has no persistent storage for variables. When you unplug it:
- All variables reset to their initial values
- `setup()` runs again from scratch
- `loop()` starts over

This is like a server that restarts on every request — your state is gone unless you explicitly save it (e.g., to EEPROM or flash storage).

---

## Resources

- [Arduino Language Reference](https://www.arduino.cc/reference/en/) — all functions documented
- [ESP32 Arduino Documentation](https://docs.espressif.com/projects/arduino-esp32/) — ESP32-specific docs
- [PlatformIO Documentation](https://docs.platformio.org/) — build system docs
- [Arduino Reddit](https://reddit.com/r/arduino) — community help
