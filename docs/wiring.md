# Wiring Guide — Step-by-Step Connections

## Important Safety Notes

- **Never connect 24V to ESP32 pins.** This will permanently damage the board.
- **Disconnect all power before wiring.** Plug in 24V only after everything is connected.
- **Double-check every wire** before powering on. A wrong connection can destroy components.
- **Do not connect USB and LM2596 simultaneously.** Use one power source at a time.
- **Common ground is critical.** The 24V supply GND, ESP32 GND, and MOSFET Source must all be connected.

---

## Parts Needed

| # | Component | Qty |
|---|-----------|-----|
| 1 | ESP-WROOM-32 DevKit V1 | 1 |
| 2 | 24V COB LED Strip | 1 |
| 3 | LD2410C mmWave Radar Sensor | 1 |
| 4 | IRLZ44N MOSFET (TO-220) | 1 |
| 5 | LM2596 Buck Converter Module | 1 |
| 6 | Potentiometer (10kΩ typical) | 1 |
| 7 | Momentary push button | 1 |
| 8 | 24V DC Power Supply | 1 |
| 9 | Protoboard (40x60mm) | 1 |
| 10 | Jumper wires (male-to-male, male-to-female) | ~10 |
| 11 | USB cable (Type-C or Micro-USB depending on board) | 1 |
| 12 | Multimeter (recommended) | 1 |

**Removed from Arduino Uno build:** 3× 1kΩ resistors (voltage divider no longer needed — ESP32 is 3.3V native, same as LD2410C).

---

## Step 1: LM2596 — Step Down 24V to 5V

The LM2596 converts the 24V supply to 5V for the ESP32.

**Before connecting to the ESP32:** Adjust the output voltage.

1. Connect 24V to the LM2596 input terminals (IN+ and IN−)
2. Do NOT connect the output to the ESP32 yet
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
| OUT+ | ESP32 VIN pin |
| OUT− | ESP32 GND |

**Warning:** Once the output is set to 5V, do NOT connect both USB and LM2596 output at the same time. Use one or the other.

---

## Step 2: IRLZ44N MOSFET

The MOSFET switches the 24V circuit using a PWM signal from the ESP32.

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
| 1 | Gate (G) | ESP32 GPIO 25 |
| 2 | Drain (D) | LED strip − (black wire) |
| 3 | Source (S) | Common GND |

**Important:** The Source pin connects to GND — the same ground as the ESP32 and the 24V supply.

---

## Step 3: Potentiometer → ESP32

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
| Left | ESP32 3.3V | Red |
| Middle | ESP32 GPIO 34 | Yellow |
| Right | ESP32 GND | Black |

**Note:** GPIO 34 is input-only and has no internal pull-up. The potentiometer provides its own voltage divider, so no pull-up is needed.

---

## Step 4: LD2410C Radar Sensor → ESP32

The LD2410C is a 24GHz mmWave radar sensor that detects both moving AND stationary humans. It communicates via UART (serial) at 256000 baud (factory default — the firmware uses it as-is).

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
| VCC | ESP32 3V3 | Powers the sensor (direct, both 3.3V) |
| TX | ESP32 GPIO 16 (UART2 RX) | Sensor sends data to ESP32 — direct connection, both 3.3V |
| RX | ESP32 GPIO 17 (UART2 TX) | ESP32 sends commands to sensor — direct connection, both 3.3V |
| OUT | (not used) | GPIO presence output — not needed with UART |
| GND | ESP32 GND | Common ground |

**No voltage divider needed!** Both the ESP32 and LD2410C operate at 3.3V. The TX/RX lines connect directly — this is a major simplification over the Arduino Uno build.

**Mounting:**
- Antenna side (copper trace side) faces the detection area
- Mount 2–3m high, slight downward angle
- Leave space behind the sensor (back lobe can detect through thin walls)
- Optional: place a metal shield behind the sensor to block back lobe detection

---

## Step 5: Button → ESP32

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
| Leg A (any) | ESP32 GPIO 27 |
| Leg B (opposite side) | ESP32 GND |

The button connects the two legs when pressed. We use `INPUT_PULLUP` in code, so no external resistor is needed.

---

## Step 6: LED Strip → 24V Circuit

This is where you switch the high voltage. **Keep these wires away from the ESP32 side wires.**

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
24V Supply − ──────┬───── ESP32 GND
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

Without a common ground, the PWM signal from the ESP32 has no reference point and the MOSFET won't switch properly.

---

## Complete Wiring Diagram

```
                    ┌──────────────────┐
                    │  ESP-WROOM-32    │
                    │  DevKit V1       │
                    │                  │
  USB ─────────────►│ USB              │  (disconnect when using LM2596)
                    │                  │
                    │ VIN ─────────────┼── LM2596 OUT+
                    │                  │
                    │ 3V3 ─────────────┼── POT Left
                    │                  │   LD2410C VCC (direct, 3.3V!)
                    │                  │
                    │ GND ─────────────┼──┬── Common GND bus
                    │                  │  │
                    │ GPIO34 ──────────┼──── POT Middle
                    │                  │
                    │ GPIO16 ──────────┼──── LD2410C TX (UART2 RX, direct!)
                    │                  │
                    │ GPIO17 ──────────┼──── LD2410C RX (UART2 TX, direct!)
                    │                  │
                    │ GPIO27 ──────────┼──── Button Leg A
                    │                  │
                     │ GPIO23 ──────────┼──── MOSFET Gate
                    │                  │
                    │ GPIO2 ───────────┼──── (built-in blue LED, mode indicator)
                    └──────────────────┘

                    ┌──────────────────┐
                    │    LM2596        │
                    │                  │
                    │  IN+ ────────────┼──── 24V Supply +
                    │  IN− ────────────┼──── 24V Supply −
                    │  OUT+ ───────────┼──── ESP32 VIN
                    │  OUT− ───────────┼──── Common GND
                    └──────────────────┘

                    ┌──────────────────┐
                    │  IRLZ44N         │
                    │  (TO-220)        │
                    │                  │
                      │  Gate ───────────┼──── ESP32 GPIO 25
                    │  Drain ──────────┼──── LED Strip −
                    │  Source ─────────┼──── Common GND
                    └──────────────────┘

                    ┌──────────────────┐
                    │  24V SUPPLY      │
                    │                  │
                    │  + ──────────────┼──── LM2596 IN+
                    │                  │     LED Strip +
                    │  − ──────────────┼──── LM2596 IN−
                    │                  │     Common GND
                    └──────────────────┘

                    ┌──────────────────┐
                    │  LED STRIP       │
                    │                  │
                    │  + ──────────────┼──── 24V Supply +
                    │  − ──────────────┼──── MOSFET Drain
                    └──────────────────┘

                    ┌──────────────────┐
                    │  LD2410C         │
                    │  RADAR           │
                    │                  │
                    │  VCC ────────────┼──── ESP32 3V3
                    │  TX ─────────────┼──── ESP32 GPIO 16 (direct!)
                    │  RX ─────────────┼──── ESP32 GPIO 17 (direct!)
                    │  GND ────────────┼──── Common GND
                    └──────────────────┘
```

---

## Before You Power On — Checklist

1. ✅ LM2596 output adjusted to 5.0V (measured with multimeter)
2. ✅ All grounds connected (24V−, ESP32 GND, MOSFET Source)
3. ✅ MOSFET Gate → ESP32 GPIO 25
4. ✅ MOSFET Drain → LED strip −
5. ✅ MOSFET Source → GND
6. ✅ LED strip + → 24V+
7. ✅ Potentiometer wired correctly (3.3V, GPIO 34, GND)
8. ✅ LD2410C wired correctly (3V3, GPIO 16, GPIO 17 — direct, no voltage divider!)
9. ✅ Button wired correctly (GPIO 27, GND)
10. ✅ No wire crosses between ESP32 side and 24V side
11. ✅ 24V supply is UNPLUGGED from wall outlet
12. ✅ USB is DISCONNECTED (using LM2596 for power)

---

## Testing Without 24V (Safe First Test)

You can test the logic before connecting the 24V supply:

1. Connect USB to ESP32 (do NOT connect 24V)
2. Connect all inputs (potentiometer, LD2410C, button)
3. Connect MOSFET Gate to GPIO 25 (leave Drain and Source disconnected)
4. Open serial monitor (`pio device monitor` — 115200 baud)
5. You should see `LED-on-presence started` and `Mode: PRESENCE`
6. The radar should initialize and show `Radar: config OK` (or `Radar: configured` on first boot)
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
