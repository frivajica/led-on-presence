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
| 3 | HC-SR501 PIR Motion Sensor | 1 |
| 4 | IRLZ44N MOSFET (TO-220) | 1 |
| 5 | LM2596 Buck Converter Module | 1 |
| 6 | Potentiometer (10kΩ typical) | 1 |
| 7 | Momentary push button | 1 |
| 8 | 24V DC Power Supply | 1 |
| 9 | Protoboard (40x60mm) | 1 |
| 10 | Jumper wires (male-to-male, male-to-female) | ~12 |
| 11 | USB cable (Type A to Type B) | 1 |
| 12 | Multimeter (recommended) | 1 |

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

## Step 4: HC-SR501 Motion Sensor → Arduino

The sensor has 3 pins sticking out. Looking at the sensor from the front (lens side):

```
    ┌───────────┐
    │  ○ Fresnel│
    │    Lens   │
    │           │
    └─┬───┬───┬─┘
      │   │   │
     VCC OUT GND
```

| Sensor Pin | Connect To | Wire Color |
|------------|-----------|------------|
| VCC | Arduino 5V | Red |
| OUT | Arduino D2 | White |
| GND | Arduino GND | Black |

**Adjust the pots on the sensor BEFORE wiring:**
- Left pot (sensitivity): turn to about 50%
- Right pot (delay): turn **fully counter-clockwise** (minimum ~0.3s) — Arduino handles the timeout

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
                    ├───── Motion Sensor GND
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
                    │              │  └── PIR VCC
                    │              │
                    │ GND ─────────┼──┬── Common GND bus
                    │              │  │
                    │ A0 ──────────┼──── POT Middle
                    │              │
                    │ D2 ──────────┼──── PIR OUT
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
8. ✅ Motion sensor wired correctly (5V, D2, GND)
9. ✅ Button wired correctly (D3, GND)
10. ✅ No wire crosses between Arduino side and 24V side
11. ✅ 24V supply is UNPLUGGED from wall outlet
12. ✅ USB is DISCONNECTED (using LM2596 for power)

---

## Testing Without 24V (Safe First Test)

You can test the logic before connecting the 24V supply:

1. Connect USB to Arduino (do NOT connect 24V)
2. Connect all inputs (potentiometer, motion sensor, button)
3. Connect MOSFET Gate to D6 (leave Drain and Source disconnected)
4. Open serial monitor (`pio device monitor`)
5. You should see `LED-on-presence started` and `Mode: MOTION`
6. Wave your hand → serial should show motion detected
7. Turn the potentiometer → serial should show brightness value changing
8. Press the button → mode should toggle to MANUAL

The `Bright:` value in serial output shows the PWM value (0–255). When it changes as you turn the pot, the MOSFET wiring is ready.

---

## MOSFET Without Heatsink

For LED strips drawing under 2A, the IRLZ44N's Rds(on) of 0.022Ω generates minimal heat:

```
Power dissipated = I² × R = (2A)² × 0.022Ω = 0.088W
```

This is negligible — no heatsink needed for this project. If you later drive higher loads (>5A), add a heatsink to the metal tab.
