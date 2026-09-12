# Components — What Each Part Does

## ESP-WROOM-32 DevKit V1

**What it is:** A small computer on a circuit board based on the ESP32 chip. It runs your code in a loop — `setup()` once, then `loop()` forever.

**Why ESP32:** Much more powerful than Arduino Uno (240 MHz vs 16 MHz), has 520KB RAM (vs 2KB), 4MB flash (vs 32KB), 34 GPIO pins, and — critically — 3.3V logic. This means the LD2410C radar sensor connects directly without a voltage divider. It also has 3 hardware UARTs, so no unreliable SoftwareSerial needed. The ESP32's FreeRTOS multi-tasking capability allows the radar's `autoReadTask()` to run in the background without blocking the main loop.

**How it works:**
- Runs at 240 MHz (15× faster than Arduino Uno)
- Has 520 KB RAM, 4 MB flash storage for your code
- Digital pins output 0V (LOW) or 3.3V (HIGH)
- Analog pins read voltages 0–3.3V as numbers 0–4095 (12-bit resolution)
- All digital pins can do PWM via LEDC or SigmaDelta peripherals
- Has built-in WiFi and Bluetooth (used for OTA updates and radar configuration)

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

**Why IRLZ44N:** It's "logic-level" — turns on with just 3.3V at the gate, which the ESP32 can provide directly. Other MOSFETs need 10V+ and won't work with microcontrollers.

**Key specs:**
- **Vds (max voltage):** 55V — easily handles 24V
- **Id (max current):** 47A — way more than the LED strip needs
- **Vgs (gate threshold):** 1–2V — begins conducting at 3.3V (ESP32's output)
- **Rds(on) (resistance when on):** ~0.028Ω at Vgs=5V; higher at 3.3V

**How it works:**
1. ESP32 sends a 2 MHz SigmaDelta signal (0–3.3V) to the Gate pin through a 220Ω series resistor
2. The MOSFET's gate capacitance (~1200 pF) combined with the 220Ω resistor filters the 2 MHz signal into a smooth DC voltage
3. When the DC gate voltage is high enough, the MOSFET conducts: current flows from Drain to Source
4. When the gate voltage is low, the MOSFET blocks: no current flows
5. The resulting average current through the LED strip creates the desired brightness

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
| 1 | Gate (G) | 220Ω series resistor → ESP32 GPIO 25 (SigmaDelta PWM) |
| 2 | Drain (D) | LED strip − |
| 3 | Source (S) | Common GND |

**Why the 220Ω series resistor is critical:**
Without it, the GPIO directly drives the gate with fast transitions, pushing the MOSFET into its linear region at 3.3V (since the gate charge/discharge is too fast for the limited gate drive voltage). The 220Ω resistor slows the gate transitions, and at 2 MHz the gate capacitance acts as a low-pass filter, producing a smooth DC gate voltage.

**SigmaDelta PWM vs traditional PWM:**
A traditional PWM signal (like LEDC) is a square wave that switches between 0% and 100% duty cycle at a fixed frequency. The MOSFET sees discrete on/off states. At only 3.3V gate drive, the IRLZ44N's transfer curve is very steep — tiny voltage fluctuations cause large current swings, creating visible flicker.

SigmaDelta is different: it's a noise-shaped signal at 2 MHz that varies the *density* of 1s and 0s. At 2 MHz, the MOSFET's own gate capacitance smooths this into a steady analog voltage — like converting a 1-bit audio stream to analog. The gate sits at a fixed DC level proportional to the brightness value, eliminating flicker entirely.

---

## 220Ω Series Resistor

**What it is:** A resistor placed in series between GPIO 25 and the MOSFET gate pin.

**Why it's critical:** Combined with the MOSFET's gate capacitance (~1200 pF), it forms an RC low-pass filter with a cutoff of ~600 kHz. At the 2 MHz SigmaDelta frequency, this filters the noise-shaped signal into a smooth DC voltage at the gate. The MOSFET sees a steady analog voltage instead of rapid switching transitions.

**Value matters:**
- **220Ω** (correct): Gate settles in ~1.3 µs, clean filtering at 2 MHz
- **1kΩ or higher** (too much): Gate voltage is too low (RC time constant too long)
- **No resistor** (wrong): Gate sees raw 2 MHz transitions → flicker

**Connection:** Must be in series between GPIO 25 and the gate pin, **not** from gate to ground. A gate-to-ground resistor creates a voltage divider that halves the gate voltage.

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

**EMI concern:** The LM2596 switches at ~100-500 kHz, radiating electromagnetic interference that couples into nearby wires — especially the UART lines (GPIO 16/17) connecting the ESP32 to the radar. This is the root cause of the `RADAR OFFLINE/ONLINE` spam seen in the Serial monitor. Moving the buck converter away from the ESP32 and UART wires (even 10cm) can reduce interference by ~90%.

**Safety note:** The LM2596 has no overcurrent or short-circuit protection. Add a fuse or use a supply with current limiting.

---

## LD2410C mmWave Radar Sensor

**What it is:** A 24GHz Frequency Modulated Continuous Wave (FMCW) radar sensor. Detects both moving AND stationary humans by measuring reflected radio waves.

**Why LD2410C:** Detects presence even when you're sitting still (unlike PIR sensors which only detect motion). Not affected by light, heat, or IR sources. Configurable detection range and sensitivity per distance gate. And most importantly — operates at 3.3V, connecting directly to the ESP32.

**Key specs:**
- **Frequency:** 24 GHz (ISM band, no license needed)
- **Detection range:** Up to ~6m (configurable via Bluetooth app)
- **Detection angle:** ~60–90°
- **Interface:** UART at 115200 baud (configured via Bluetooth app)
- **Voltage:** 3.3V (with onboard regulator, accepts up to 5V on VCC)
- **Current:** ~80mA typical

**How it works:**
1. Emits 24GHz radio waves
2. Waves bounce off objects and return to the antenna
3. The frequency shift (Doppler effect) reveals motion
4. The time delay reveals distance
5. Internal chip processes signal and reports targets via UART

**Background task:** The `autoReadTask()` FreeRTOS task runs continuously in the background, draining UART bytes and parsing data frames. The main loop reads the sensor's cached state — no blocking UART reads needed.

**EMI sensitivity:** The buck converter's switching noise can corrupt UART frames into ghost presence reports. These corrupted frames always report `distance = 0cm` (physically impossible — the sensor's minimum detection range is ~75cm). The software filters these out by rejecting any presence report with zero distance.

**Why it's better than PIR (HC-SR501):**
- Detects stationary people (PIR only detects motion)
- Not affected by temperature, sunlight, or IR sources
- Can detect through thin walls
- Configurable sensitivity per distance gate
- More reliable presence detection

**UART communication:**
The sensor outputs data frames continuously (~1 frame per second). The ESP32's `autoReadTask()` reads and parses these in a background FreeRTOS task. The library's `isConnected()` checks whether a valid frame arrived within the last 100ms — if not, the radar is considered "offline."

**Mounting:**
- Antenna side (copper trace side) faces the detection area
- Mount 2–3m high, slight downward angle
- Leave space behind the sensor (back lobe can detect through thin walls)
- Optional: place a metal shield behind the sensor to block back lobe detection

---

## Potentiometer

**What it is:** A variable resistor. Turning the knob changes the resistance, which changes the voltage at the middle pin.

**Why:** Controls brightness. In presence mode, sets the maximum brightness. In manual mode, directly controls brightness.

**How it works:**
- Outer pin 1 → 3.3V
- Outer pin 2 → GND
- Middle pin (wiper) → ESP32 GPIO 34

As you turn the knob:
- Fully toward 3.3V pin → GPIO 34 reads ~1023 → full brightness
- Fully toward GND pin → GPIO 34 reads ~0 → off
- Middle → GPIO 34 reads ~512 → 50% brightness

**Triple noise filtering:**
The ESP32's ADC has ~50-100 LSB of thermal noise. To produce a stable reading, the potentiometer value passes through three layers of filtering:

1. **8-sample averaging** per read (~100 µs total)
2. **Exponential Moving Average** (α=0.15) across reads (~13-sample equivalent)
3. **Hysteresis** (±3 units): brightness target only updates when change exceeds 3/255

This eliminates the fast brightness fluctuations caused by power rail noise from the buck converter.

---

## Button

**What it is:** A momentary push button (tactile switch). Pressing it connects two pins.

**Why:** Toggle between presence mode and manual mode.

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
Person enters room
    → LD2410C radar detects target via 24GHz radio waves
        → autoReadTask() (FreeRTOS background) parses UART frame
            → radar.presenceDetected() + distance > 0 filter (EMI rejection)
                → 100ms detection debounce (bidirectional)
                    → effectivePresence = true
                        → ESP32 computes target brightness from EMA-filtered pot
                            → Brightness hysteresis check (±3)
                                → Fade interpolation (time-based, duration scales with brightness)
                                    → sigmaDeltaWrite() at 2 MHz (write guard — only if changed)
                                        → 220Ω series resistor + gate capacitance
                                            → Smooth DC voltage at MOSFET gate
                                                → MOSFET conducts → LED strip lights

Person leaves room
    → Radar stops reporting target (after "unmanned" duration)
        → 100ms loss debounce
            → 15-second countdown begins
                → Light stays on during countdown
                    → Countdown complete → light fades off

EMI from buck converter
    → UART frame corruption → ghost presence report with 0cm distance
        → EMI filter rejects (distance must be > 0)
            → No false trigger
```
