# Wiring Guide — Step-by-Step Connections

## Important Safety Notes

- **Never connect 24V to Arduino pins.** This will permanently damage the board.
- **Disconnect all power before wiring.** Plug in 24V only after everything is connected.
- **Double-check every wire** before powering on. A wrong connection can destroy components.
- **Do not connect USB and LM2596 simultaneously.** Use one power source at a time.
- **Common ground is critical.** The 24V supply GND, Arduino GND, and MOSFET Source must all be connected.

---

## Parts Needed

| # | Component | Qty |
|---|-----------|-----|
| 1 | Arduino Uno R3 | 1 |
| 2 | 24V COB LED Strip | 1 |
| 3 | LD2410C mmWave Radar Sensor | 1 |
| 4 | IRLZ44N MOSFET (TO-220) | 1 |
| 5 | LM2596 Buck Converter Module | 1 |
| 6 | Potentiometer (10kΩ typical) | 1 |
| 7 | Momentary push button | 1 |
| 8 | 24V DC Power Supply | 1 |
| 9 | Protoboard (40x60mm) | 1 |
| 10 | Resistors: 1kΩ, 2kΩ (voltage divider) | 2 |
| 11 | Jumper wires (male-to-male, male-to-female) | ~14 |
| 12 | USB cable (Type A to Type B) | 1 |
| 13 | Multimeter (recommended) | 1 |

---

## Step 1: LM2596 — Step Down 24V to 5V

The LM2596 converts the 24V supply to 5V for the Arduino.

**Before connecting to the Arduino:** Adjust the output voltage.

1. Connect 24V to the LM2596 input terminals (IN+ and IN−)
2. Do NOT connect the output to the Arduino yet
3. Use a multimeter to measure the output voltage (OUT+ and OUT−)
4. Turn the small trim pot on the LM2596 until the multimeter reads **5.0V**
5. Disconnect the 24V input

```
    ┌──────────────────────┐
    │      LM2596          │
    │                      │
    │  IN+  IN−  OUT+ OUT− │
    │   ●    ●    ●    ●  │
    │                      │
    │     [trim pot]       │
    └──────────────────────┘
```

| LM2596 Pin | Connect To |
|------------|-----------|
| IN+ | 24V Power Supply + |
| IN− | 24V Power Supply − |
| OUT+ | Arduino 5V pin |
| OUT− | Arduino GND |

**Warning:** Once the output is set to 5V, do NOT connect both USB and LM2596 output at the same time. Use one or the other.

---

## Step 2: IRLZ44N MOSFET

The MOSFET switches the 24V circuit using a PWM signal from the Arduino.

**Pin identification:** Hold the TO-220 package with the flat metal face toward you and pins pointing down:

```
    ┌─────────┐
    │         │
    │  Metal  │
    │  Tab    │
    │         │
    └┬──┬──┬─┘
     │  │  │
     1  2  3
     G  D  S
```

| MOSFET Pin | Name | Connect To |
|------------|------|-----------|
| 1 | Gate (G) | Arduino D6 |
| 2 | Drain (D) | LED strip − (black wire) |
| 3 | Source (S) | Common GND |

**Important:** The Source pin connects to GND — the same ground as the Arduino and the 24V supply.

---

## Step 3: Potentiometer → Arduino

The potentiometer has 3 pins. Hold it with the knob facing you and the pins facing down:

```
       ┌─────────┐
       │   POT   │
       │         │
  ┌────┤  ○   ○  ├────┐
  │    │    ▲    │    │
  │    │    │    │    │
  │    └────┼────┘    │
  │         │         │
 Left     Middle    Right
 (CCW)    (Wiper)   (CW)
```

| Pot Pin | Connect To | Wire Color (suggested) |
|---------|-----------|----------------------|
| Left | Arduino 5V | Red |
| Middle | Arduino A0 | Yellow |
| Right | Arduino GND | Black |

---

## Step 4: LD2410C Radar Sensor → Arduino

The LD2410C is a 24GHz mmWave radar sensor that detects both moving AND stationary humans. It communicates via UART (serial) at 38400 baud.

**Pin identification:** The LD2410C has 5 pins on one end of the PCB:

```
    LD2410C PCB (antenna side facing up):

    ┌──────────────────────┐
    │ ≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈ │ ← Antenna (ACTIVE detection face)
    │ ≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈≈ │
    │                      │
    │  [chip] [chip]       │
    └──┬───┬───┬───┬───┬───┘
       VCC TX  RX  OUT GND
```

| Sensor Pin | Connect To | Notes |
|------------|-----------|-------|
| VCC | Arduino 5V | Powers the sensor |
| TX | Arduino Pin 10 (via SoftwareSerial RX) | Sensor sends data to Arduino |
| RX | Arduino Pin 11 (via voltage divider) | Arduino sends commands to sensor |
| OUT | (not used) | GPIO presence output — not needed with UART |
| GND | Arduino GND | Common ground |

**Voltage divider required on TX→RX line:**

The LD2410C RX pin is 3.3V. Arduino TX is 5V. You must drop the voltage:

```
Arduino Pin 11 ────[1kΩ]────┬────[2kΩ]──── GND
                             │
                             └────→ LD2410C RX (3.3V)
```

**How it works:**
- Arduino sends 5V signal through the divider
- Output is 5V × (2kΩ / (1kΩ + 2kΩ)) = 3.33V — safe for the sensor
- The sensor TX (3.3V) can connect directly to Arduino RX — 3.3V reads as HIGH on a 5V Arduino

**First boot baud rate change:**

The LD2410C ships at 256000 baud. The Arduino code automatically changes it to 38400 during setup:
1. Connects at 256000 temporarily
2. Sends `setBaudRate(38400)` command
3. Reconnects at 38400 for normal operation

**Mounting:**
- Antenna side (copper trace side) faces the detection area
- Mount 2–3m high, slight downward angle
- Leave space behind the sensor (back lobe can detect through thin walls)
- Optional: place a metal shield behind the sensor to block back lobe detection

---

## Step 5: Button → Arduino

```
    ┌─────────┐
    │ BUTTON  │
    │  ┌───┐  │
    │  │   │  │
    └──┤   ├──┘
       │   │
    ┌──┘   └──┐
    │         │
  Leg A     Leg B
```

| Button Leg | Connect To |
|------------|-----------|
| Leg A (any) | Arduino D3 |
| Leg B (opposite side) | Arduino GND |

The button connects the two legs when pressed. We use `INPUT_PULLUP` in code, so no external resistor is needed.

---

## Step 6: LED Strip → 24V Circuit

This is where you switch the high voltage. **Keep these wires away from the Arduino side wires.**

**Complete 24V circuit:**

```
24V Supply + ──→ LED Strip +
                      │
                [LED strip LEDs]
                      │
                MOSFET Drain
                      │
                MOSFET Source
                      │
24V Supply − ──→ Common GND
```

| Connection | Wire |
|------------|------|
| 24V Supply + | LED Strip + (red wire) |
| LED Strip − (black wire) | MOSFET Drain (pin 2) |
| MOSFET Source (pin 3) | Common GND |
| 24V Supply − | Common GND |

When the MOSFET is OFF (Gate = LOW), no current flows.
When the MOSFET is ON (Gate = HIGH), current flows from 24V+ through the LED strip, through the MOSFET, to GND.

---

## Step 7: Common Ground

This is the most critical connection. **All grounds must be connected:**

```
24V Supply − ──────┬───── Arduino GND
                    │
                    ├───── LM2596 OUT−
                    │
                    ├───── MOSFET Source (pin 3)
                    │
                    ├───── Potentiometer Right pin
                    │
                    ├───── LD2410C GND
                    │
                    └───── Button Leg B
```

Without a common ground, the PWM signal from the Arduino has no reference point and the MOSFET won't switch properly.

---

## Complete Wiring Diagram

```
                    ┌──────────────┐
                    │  ARDUINO UNO │
                    │              │
  USB ─────────────►│ USB          │  (disconnect when using LM2596)
                    │              │
                    │ 5V ──────────┼──┬── LM2596 OUT+
                    │              │  ├── POT Left
                    │              │  └── LD2410C VCC
                    │              │
                    │ GND ─────────┼──┬── Common GND bus
                    │              │  │
                    │ A0 ──────────┼──── POT Middle
                    │              │
                    │ 10 ──────────┼──── LD2410C TX (via SoftwareSerial RX)
                    │              │
                    │ 11 ──────────┼──── LD2410C RX (via voltage divider!)
                    │              │
                    │ D3 ──────────┼──── Button Leg A
                    │              │
                    │ D6 ──────────┼──── MOSFET Gate
                    │              │
                    │ D13 ─────────┼──── (built-in LED, mode indicator)
                    └──────────────┘

                    ┌──────────────┐
                    │    LM2596    │
                    │              │
                    │  IN+ ────────┼──── 24V Supply +
                    │  IN− ────────┼──── 24V Supply −
                    │  OUT+ ───────┼──── Arduino 5V
                    │  OUT− ───────┼──── Common GND
                    └──────────────┘

                    ┌──────────────┐
                    │  IRLZ44N     │
                    │  (TO-220)    │
                    │              │
                    │  Gate ───────┼──── Arduino D6
                    │  Drain ──────┼──── LED Strip −
                    │  Source ─────┼──── Common GND
                    └──────────────┘

                    ┌──────────────┐
                    │  24V SUPPLY  │
                    │              │
                    │  + ──────────┼──── LM2596 IN+
                    │              │     LED Strip +
                    │  − ──────────┼──── LM2596 IN−
                    │              │     Common GND
                    └──────────────┘

                    ┌──────────────┐
                    │  LED STRIP   │
                    │              │
                    │  + ──────────┼──── 24V Supply +
                    │  − ──────────┼──── MOSFET Drain
                    └──────────────┘

                    ┌──────────────┐
                    │  LD2410C     │
                    │  RADAR       │
                    │              │
                    │  VCC ────────┼──── Arduino 5V
                    │  TX ─────────┼──── Arduino Pin 10
                    │  RX ─────────┼──── Arduino Pin 11 (via voltage divider!)
                    │  GND ────────┼──── Common GND
                    └──────────────┘
```

---

## Before You Power On — Checklist

1. ✅ LM2596 output adjusted to 5.0V (measured with multimeter)
2. ✅ All grounds connected (24V−, Arduino GND, MOSFET Source)
3. ✅ MOSFET Gate → Arduino D6
4. ✅ MOSFET Drain → LED strip −
5. ✅ MOSFET Source → GND
6. ✅ LED strip + → 24V+
7. ✅ Potentiometer wired correctly (5V, A0, GND)
8. ✅ LD2410C wired correctly (5V, Pin 10, Pin 11 with voltage divider, GND)
9. ✅ Voltage divider on Pin11 → LD2410C RX (1kΩ + 2kΩ)
10. ✅ Button wired correctly (D3, GND)
11. ✅ No wire crosses between Arduino side and 24V side
12. ✅ 24V supply is UNPLUGGED from wall outlet
13. ✅ USB is DISCONNECTED (using LM2596 for power)

---

## Testing Without 24V (Safe First Test)

You can test the logic before connecting the 24V supply:

1. Connect USB to Arduino (do NOT connect 24V)
2. Connect all inputs (potentiometer, LD2410C, button)
3. Connect MOSFET Gate to D6 (leave Drain and Source disconnected)
4. Open serial monitor (`pio device monitor`)
5. You should see `LED-on-presence started` and `Mode: MOTION`
6. The radar will initialize at 256000, change to 38400, then show `Radar: connected`
7. Walk in front of sensor → serial should show `Pres: Y` with distance
8. Turn the potentiometer → serial should show brightness value changing
9. Press the button → mode should toggle to MANUAL

The `Bright:` value in serial output shows the PWM value (0–255). When it changes as you turn the pot, the MOSFET wiring is ready.

---

## MOSFET Without Heatsink

For LED strips drawing under 2A, the IRLZ44N's Rds(on) of 0.022Ω generates minimal heat:

```
Power dissipated = I² × R = (2A)² × 0.022Ω = 0.088W
```

This is negligible — no heatsink needed for this project. If you later drive higher loads (>5A), add a heatsink to the metal tab.
