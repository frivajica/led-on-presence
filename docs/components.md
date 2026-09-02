# Components — What Each Part Does

## ESP-WROOM-32 DevKit V1

**What it is:** A small computer on a circuit board based on the ESP32 chip. It runs your code in a loop — `setup()` once, then `loop()` forever.

**Why ESP32:** Much more powerful than Arduino Uno (240 MHz vs 16 MHz), has 520KB RAM (vs 2KB), 4MB flash (vs 32KB), 34 GPIO pins, and — critically — 3.3V logic. This means the LD2410C radar sensor connects directly without a voltage divider. It also has 3 hardware UARTs, so no unreliable SoftwareSerial needed.

**How it works:**
- Runs at 240 MHz (15× faster than Arduino Uno)
- Has 520 KB RAM, 4 MB flash storage for your code
- Digital pins output 0V (LOW) or 3.3V (HIGH)
- Analog pins read voltages 0–3.3V as numbers 0–4095 (12-bit resolution)
- All digital pins can do PWM via LEDC channels
- Has built-in WiFi and Bluetooth (not used in this project)

**Key concept — voltage levels:**
ESP32 operates at 3.3V. Your LED strip operates at 24V. You **cannot** connect 24V directly to an ESP32 pin — it will destroy it. The MOSFET acts as a bridge: it switches the 24V circuit using a 3.3V signal from the ESP32.

**Key difference from Arduino Uno:**
The ESP32 is 3.3V, not 5V. This is actually better for this project because the LD2410C is also 3.3V — no voltage divider needed on the UART lines.

---

## 24V COB LED Strip (Lumiora)

**What it is:** A strip of Chip-on-Board LEDs. COB means many tiny LED chips are mounted on a single substrate, creating a smooth, continuous line of light (no visible dots like older strips).

**Why COB:** Uniform light output, no hotspots, looks premium.

**Key specs:**
- **24V DC** — needs a 24V power supply
- **Analog** — all LEDs are on/off together (not individually addressable)
- **Dimmable** — via PWM signal to a MOSFET

**How it works electrically:**
The strip draws current from the 24V supply. The ESP32 cannot supply this power — the ESP32's 3.3V pin can only provide ~400mA, while the strip might draw several amps. The 24V supply powers the strip directly through the MOSFET; the ESP32 just tells the MOSFET how fast to switch.

---

## IRLZ44N MOSFET

**What it is:** A logic-level N-channel MOSFET. Acts as an electrically controlled switch that can handle high voltage and current.

**Why IRLZ44N:** It's "logic-level" — fully turns on with just 3.3V at the gate, which the ESP32 can provide directly. Other MOSFETs need 10V+ and won't work with microcontrollers.

**Key specs:**
- **Vds (max voltage):** 55V — easily handles 24V
- **Id (max current):** 47A — way more than the LED strip needs
- **Vgs (gate threshold):** 1–2V — turns on fully at 3.3V (ESP32's output)
- **Rds(on) (resistance when on):** ~0.022Ω — very low, minimal heat

**How it works:**
1. ESP32 sends a PWM signal (0–3.3V) to the Gate pin
2. When Gate is HIGH (3.3V), the MOSFET conducts: current flows from Drain to Source
3. When Gate is LOW (0V), the MOSFET blocks: no current flows
4. PWM switches this on/off thousands of times per second, creating an average voltage that dims the LED

**Pinout (TO-220 package, flat face toward you):**

```
    ┌─────────┐
    │         │
    │  Metal  │
    │  Tab    │
    │         │
    └┬──┬──┬─┘
     │  │  │
     G  D  S
```

| Pin | Name | Connects To |
|-----|------|------------|
| 1 | Gate (G) | ESP32 GPIO 25 (PWM) |
| 2 | Drain (D) | LED strip − |
| 3 | Source (S) | Common GND |

**Why PWM matters:**
A relay can only do ON/OFF. A MOSFET with PWM can simulate any voltage between 0V and 24V by switching very fast:
- `analogWrite(pin, 0)` → 0% duty cycle → LED off
- `analogWrite(pin, 127)` → 50% duty cycle → LED at half brightness
- `analogWrite(pin, 255)` → 100% duty cycle → LED at full brightness

---

## LM2596 Buck Converter

**What it is:** A DC-DC step-down converter. Takes a higher voltage and converts it to a lower, regulated voltage.

**Why LM2596:** Cheap (~$1), efficient (~80%), handles up to 3A output, adjustable output voltage via trim pot.

**Key specs:**
- **Input voltage:** 4.5V – 40V
- **Output voltage:** 1.25V – 35V (adjustable)
- **Output current:** Up to 3A (with adequate heat sinking)
- **Efficiency:** ~80% (much better than a linear regulator)

**How it works:**
1. Connect 24V from the power supply to the input terminals
2. Adjust the onboard trim pot (small screw) until the output reads 5V with a multimeter
3. Connect output to ESP32's VIN pin

**Why not use a linear regulator (like 7805)?**
A linear regulator dissipates the voltage difference as heat. Dropping 24V to 5V means dissipating 19V × current as heat. At just 100mA, that's 1.9W — hot enough to burn your finger. The LM2596 switches instead of burning, so it stays cool.

**Safety note:** The LM2596 has no overcurrent or short-circuit protection. Add a fuse or use a supply with current limiting.

---

## LD2410C mmWave Radar Sensor

**What it is:** A 24GHz Frequency Modulated Continuous Wave (FMCW) radar sensor. Detects both moving AND stationary humans by measuring reflected radio waves.

**Why LD2410C:** Detects presence even when you're sitting still (unlike PIR sensors which only detect motion). Not affected by light, heat, or IR sources. Configurable detection range. And most importantly — operates at 3.3V, connecting directly to the ESP32.

**Key specs:**
- **Frequency:** 24 GHz (ISM band, no license needed)
- **Detection range:** Up to ~6m (configurable)
- **Detection angle:** ~60–90°
- **Interface:** UART at 256000 baud (factory default)
- **Voltage:** 3.3V (with onboard regulator, accepts up to 5V on VCC)
- **Current:** ~80mA typical

**How it works:**
1. Emits 24GHz radio waves
2. Waves bounce off objects and return to the antenna
3. The frequency shift (Doppler effect) reveals motion
4. The time delay reveals distance
5. Internal chip processes signal and reports targets via UART

**Why it's better than PIR (HC-SR501):**
- Detects stationary people (PIR only detects motion)
- Not affected by temperature, sunlight, or IR sources
- Can detect through thin walls
- Configurable sensitivity per distance gate
- More reliable presence detection

**UART communication:**
The sensor outputs data frames automatically. The ESP32 reads these frames to determine if someone is present, how far away they are, and whether they're moving or stationary.

**Mounting:**
- Antenna side (copper trace side) faces the detection area
- Mount 2–3m high, slight downward angle
- Leave space behind the sensor (back lobe can detect through thin walls)
- Optional: place a metal shield behind the sensor to block back lobe detection

---

## Potentiometer

**What it is:** A variable resistor. Turning the knob changes the resistance, which changes the voltage at the middle pin.

**Why:** Controls brightness. In motion mode, sets the maximum brightness. In manual mode, directly controls brightness.

**How it works:**
- Outer pin 1 → 3.3V
- Outer pin 2 → GND
- Middle pin (wiper) → ESP32 GPIO 34

As you turn the knob:
- Fully toward 3.3V pin → GPIO 34 reads ~1023 → full brightness
- Fully toward GND pin → GPIO 34 reads ~0 → off
- Middle → GPIO 34 reads ~512 → 50% brightness

The ESP32's `analogRead()` converts this voltage (0–3.3V) to a number (0–1023). `map()` then scales it to 0–255 for PWM output.

---

## Button

**What it is:** A momentary push button (tactile switch). Pressing it connects two pins.

**Why:** Toggle between motion-sensor mode and manual mode.

**How it works with INPUT_PULLUP:**

```
Button leg 1 → ESP32 GPIO 27
Button leg 2 → GND
```

- Button **not pressed**: GPIO 27 reads HIGH (pulled up internally)
- Button **pressed**: GPIO 27 reads LOW (connected to GND through the button)

**Debounce:** Mechanical buttons "bounce" — when pressed, the metal contacts vibrate and create rapid HIGH/LOW transitions for a few milliseconds. The debounce code waits 50ms and re-reads the pin to confirm the press is real.

---

## 24V Power Supply

**What it is:** Converts mains AC power (120V/240V) to 24V DC for the LED strip and (via LM2596) the ESP32.

**Current requirement:** Check your strip's wattage. A 5-meter 24V COB strip might draw 20–40W, which at 24V is ~1–1.7A. Your supply should be rated for at least 20% more than the strip's maximum draw.

---

## Summary: The Signal Chain

```
Motion detected
    → LD2410C radar detects presence via UART
        → ESP32 GPIO 16 reads data frame
            → ESP32 sets target brightness from potentiometer
                → ESP32 ramps currentBrightness toward target
                    → ESP32 outputs PWM on GPIO 25 (0-3.3V, varying duty cycle)
                        → IRLZ44N Gate receives PWM signal
                            → MOSFET switches 24V circuit at PWM frequency
                                → LED strip receives average voltage = dimmed light

Button pressed
    → ESP32 GPIO 27 reads LOW
        → Mode toggles (MOTION ↔ MANUAL)

Potentiometer turned
    → ESP32 GPIO 34 reads 0–1023
        → mapped to 0–255 for PWM output
            → brightness changes smoothly via fade
```
