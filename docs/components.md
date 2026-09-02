# Components — What Each Part Does

## Arduino Uno R3

**What it is:** A small computer on a circuit board. It runs your code in a loop — `setup()` once, then `loop()` forever.

**Why Uno:** It's the most beginner-friendly Arduino. 14 digital pins, 6 analog inputs, 5V logic, PWM on 6 pins. More than enough for this project.

**How it works:**
- Runs at 16 MHz (slow compared to a Mac, but fine for blinking lights)
- Has 2 KB RAM, 32 KB flash storage for your code
- Digital pins output 0V (LOW) or 5V (HIGH)
- Analog pins (A0–A5) read voltages 0–5V as numbers 0–1023 (10-bit resolution)
- PWM pins (3, 5, 6, 9, 10, 11) can output variable brightness via `analogWrite()`

**Key concept — voltage levels:**
Arduino operates at 5V. Your LED strip operates at 24V. You **cannot** connect 24V directly to an Arduino pin — it will destroy it. The MOSFET acts as a bridge: it switches the 24V circuit using a 5V signal from the Arduino.

---

## 24V COB LED Strip (Lumiora)

**What it is:** A strip of Chip-on-Board LEDs. COB means many tiny LED chips are mounted on a single substrate, creating a smooth, continuous line of light (no visible dots like older strips).

**Why COB:** Uniform light output, no hotspots, looks premium.

**Key specs:**
- **24V DC** — needs a 24V power supply
- **Analog** — all LEDs are on/off together (not individually addressable)
- **Dimmable** — via PWM signal to a MOSFET

**How it works electrically:**
The strip draws current from the 24V supply. The Arduino cannot supply this power — the Arduino's 5V pin can only provide ~400mA, while the strip might draw several amps. The 24V supply powers the strip directly through the MOSFET; the Arduino just tells the MOSFET how fast to switch.

---

## IRLZ44N MOSFET

**What it is:** A logic-level N-channel MOSFET. Acts as an electrically controlled switch that can handle high voltage and current.

**Why IRLZ44N:** It's "logic-level" — fully turns on with just 5V at the gate, which the Arduino can provide directly. Other MOSFETs need 10V+ and won't work with Arduino.

**Key specs:**
- **Vds (max voltage):** 55V — easily handles 24V
- **Id (max current):** 47A — way more than the LED strip needs
- **Vgs (gate threshold):** 1–2V — turns on fully at 5V (Arduino's output)
- **Rds(on) (resistance when on):** ~0.022Ω — very low, minimal heat

**How it works:**
1. Arduino sends a PWM signal (0–5V) to the Gate pin
2. When Gate is HIGH (5V), the MOSFET conducts: current flows from Drain to Source
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
| 1 | Gate (G) | Arduino D6 (PWM) |
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
3. Connect output to Arduino's 5V pin

**Why not use a linear regulator (like 7805)?**
A linear regulator dissipates the voltage difference as heat. Dropping 24V to 5V means dissipating 19V × current as heat. At just 100mA, that's 1.9W — hot enough to burn your finger. The LM2596 switches instead of burning, so it stays cool.

**Safety note:** The LM2596 has no overcurrent or short-circuit protection. Add a fuse or use a supply with current limiting.

---

## HC-SR501 PIR Motion Sensor

**What it is:** Passive Infrared sensor. Detects movement by measuring changes in infrared radiation (heat) from objects in its field of view.

**Why HC-SR501:** Cheap (~$1), reliable, adjustable sensitivity, works at 5V.

**How it works:**
1. A pyroelectric sensor detects infrared radiation changes
2. A Fresnel lens focuses IR light onto the sensor (the white dome)
3. A BISS0001 chip processes the signal
4. Output goes HIGH (3.3V) when motion detected, LOW when no motion

**On-board adjustments:**

| Pot | Controls | Range |
|-----|----------|-------|
| Left (sensitivity) | Detection range | 3–7 meters |
| Right (delay) | How long output stays HIGH after motion | 0.3s – 200s |

**For this project:** Set the right pot to minimum (fully CCW). The Arduino handles the 30-second timeout logic, which is more precise and resettable than the sensor's built-in delay.

**Trigger modes (solder jumper on back):**
- **H (Repeatable/Continuous):** Output stays HIGH as long as motion continues, then stays HIGH for the delay time after last motion.
- **L (Single/Non-repeatable):** Output goes HIGH once when motion detected, stays HIGH for the delay time, then goes LOW.

Use **H mode** — the Arduino reads the sensor continuously and manages timing itself.

**Important:** The output is 3.3V, not 5V. Arduino's digital pins read >3V as HIGH, so this works fine.

---

## Potentiometer

**What it is:** A variable resistor. Turning the knob changes the resistance, which changes the voltage at the middle pin.

**Why:** Controls brightness. In motion mode, sets the maximum brightness. In manual mode, directly controls brightness.

**How it works:**
- Outer pin 1 → 5V
- Outer pin 2 → GND
- Middle pin (wiper) → Arduino A0

As you turn the knob:
- Fully toward 5V pin → A0 reads ~1023 → full brightness
- Fully toward GND pin → A0 reads ~0 → off
- Middle → A0 reads ~512 → 50% brightness

The Arduino's `analogRead()` converts this voltage (0–5V) to a number (0–1023). `map()` then scales it to 0–255 for PWM output.

---

## Button

**What it is:** A momentary push button (tactile switch). Pressing it connects two pins.

**Why:** Toggle between motion-sensor mode and manual mode.

**How it works with INPUT_PULLUP:**

```
Button leg 1 → Arduino D3
Button leg 2 → GND
```

- Button **not pressed**: D3 reads HIGH (pulled up internally)
- Button **pressed**: D3 reads LOW (connected to GND through the button)

**Debounce:** Mechanical buttons "bounce" — when pressed, the metal contacts vibrate and create rapid HIGH/LOW transitions for a few milliseconds. The debounce code waits 50ms and re-reads the pin to confirm the press is real.

---

## 24V Power Supply

**What it is:** Converts mains AC power (120V/240V) to 24V DC for the LED strip and (via LM2596) the Arduino.

**Current requirement:** Check your strip's wattage. A 5-meter 24V COB strip might draw 20–40W, which at 24V is ~1–1.7A. Your supply should be rated for at least 20% more than the strip's maximum draw.

---

## Summary: The Signal Chain

```
Motion detected
    → HC-SR501 output goes HIGH (3.3V)
        → Arduino D2 reads HIGH
            → Arduino sets target brightness from potentiometer
                → Arduino ramps currentBrightness toward target
                    → Arduino outputs PWM on D6 (0-5V, varying duty cycle)
                        → IRLZ44N Gate receives PWM signal
                            → MOSFET switches 24V circuit at PWM frequency
                                → LED strip receives average voltage = dimmed light

Button pressed
    → Arduino D3 reads LOW
        → Mode toggles (MOTION ↔ MANUAL)

Potentiometer turned
    → Arduino A0 reads 0–1023
        → mapped to 0–255 for PWM output
            → brightness changes smoothly via fade
```
