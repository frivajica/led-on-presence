# Breadboard Guide — Prototyping Without Soldering

## What Is a Breadboard?

A breadboard is a plastic board with lots of tiny holes (called "tie points") where you plug in components and wires. Inside the board, metal clips connect certain holes together. You build circuits by plugging things in — no soldering needed.

The name is a historical joke: early electronics hobbyists literally soldered circuits onto bread (the food). The plastic boards replaced that, but the name stuck.

---

## How Your Breadboard Is Arranged

Your breadboard has 60 rows, with power rails running vertically on both sides:

```
    +−         a  b  c  d  e  │  f  g  h  i  j         +−
    rails                      │                      rails
     ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○    Row 1
     ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○    Row 2
     ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○    Row 3
     ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○    Row 4
     ...                                                  ...
     ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○    Row 60

    ←24 holes→←──── 10 holes ────→←──── 10 holes ────→←24 holes→
     (+ and −              (terminal strips)              (+ and −
      vertically)                                             vertically)
```

There are **4 vertical power rails** (two on each side) and **60 horizontal rows** in the center.

---

## How Connections Work

Inside the breadboard, metal clips connect certain holes together:

### Rows (Horizontal — Connected)

Each row of 10 holes (a–j) is connected across the center divider:

```
    a   b   c   d   e   │   f   g   h   i   j
    ○───○───○───○───○───┼───○───○───○───○───○     ← Row 1: all 10 connected
    ○───○───○───○───○───┼───○───○───○───○───○     ← Row 2: all 10 connected
    ○───○───○───○───○───┼───○───○─○─○───○───○     ← Row 3: all 10 connected
```

**Important:** On your breadboard, **both halves are connected across the center divider**. So `a1` and `f1` are electrically the same point. This is different from some breadboards where the center divides the connections.

If you plug a wire into hole `a1` and another wire into hole `f1`, they're electrically connected — same as soldering them together.

### Power Rails (Vertical — Connected)

The long strips along the sides run the full height of the board:

```
    +  ○                                              ○  +
       ○                                              ○
       ○                                              ○
       ○                                              ○
       ...                                          ...
       ○                                              ○
       ○                                              ○
    −  ○                                              ○  −
       ↑                                              ↑
       All + holes connected                  All + holes connected
       All − holes connected                  All − holes connected
```

The `+` rail is for 5V power. The `−` rail is for GND. You only need to connect power to these rails once — every hole along that rail is then available.

### Center Divider (Connected Across)

On your breadboard, the center divider does **not** split the rows — `a1` through `j1` are all connected. The divider is just a physical gap to make it easier to straddle ICs.

---

## The Coordinate System

Breadboard holes are referenced by letter + number:

```
    +−      a   b   c   d   e   │   f   g   h   i   j      +−
            ─────────────────────┼─────────────────────
            a1  b1  c1  d1  e1  │  f1  g1  h1  i1  j1
            a2  b2  c2  d2  e2  │  f2  g2  h2  i2  j2
            a3  b3  c3  d3  e3  │  f3  g3  h3  i3  j3
            ...                 │  ...
            a60 b60 c60 d60 e60│  f60 g60 h60 i60 j60
```

- Letters `a–e` = left half
- Letters `f–j` = right half
- Numbers `1–60` = rows

---

## Pin Names on the Arduino

The docs use "D3", "D6", etc., but your physical Arduino board just shows **bare numbers**:

```
Code says:          Board shows:
  D3          →        3
  D6          →        6
  D2          →        2
  A0          →       A0
```

The "D" means **digital** (pins 0–13). The "A" means **analog** (pins A0–A5). The board doesn't print the prefix — just the number.

Pins with a **~** symbol next to them (3, 5, 6, 9, 10, 11) are PWM-capable — they can do dimming, not just ON/OFF.

---

## Breadboard Layout for This Project

Here's where every component goes. The Arduino sits to the left of the breadboard, and components are arranged in rows 1–10.

```
            ARDUINO UNO
   ┌──────────────────────────────┐
   │  6    3    2    A0   5V  GND│
   │  ○    ○    ○    ○    ○   ○  │
   └──┼────┼────┼────┼────┼───┼──┘
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
      │    │    │    │    │   │
   +− │    │    │    │    │   │    +−
    ○ ┤    │    │    │    │   │    ┤ ○
    ○ ┤    │    │    │    │   │    ┤ ○
    ○ ┤    │    │    │    │   │    ┤ ○
    ○ ┤    │    │    │    │   │    ┤ ○
    ○ ┤    │    │    │    │   │    ┤ ○
    ○ ┤    │    │    │    │   │    ┤ ○
    ○ ┤  ┌─┼────┼────┼────┼───┼──┐ ┤ ○
    ○ ┤  │a│b  c│d  e│f  g│h  i│j│ ┤ ○  Row 1  ← POT Left (5V)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 2  ← POT Middle (A0)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 3  ← POT Right (GND)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 4
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 5  ← Button Leg A (D3)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 6  ← Button Leg B (GND)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 7
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 8  ← MOSFET Gate (D6)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 9  ← MOSFET Drain (LED−)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○  Row 10 ← MOSFET Source (GND)
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○
    ○ ┤  │○│○  ○│○  ○│○  ○│○  ○│○│ ┤ ○
    ○ ┤  └─┼────┼────┼────┼───┼──┘ ┤ ○
    ○ ┤    │    │    │    │   │    ┤ ○
    ...    ...                       ...
    ○ ┤    │    │    │    │   │    ┤ ○
    ○ ┤    │    │    │    │   │    ┤ ○
   +− │    │    │    │    │   │    +−
      │    │    │    │    │   │
```

---

## Step-by-Step: Place the Components

### Step 1: Power Rails

Connect Arduino power to the breadboard rails. Use the **left-side rails** (closest to the Arduino):

```
Arduino 5V  ──────────────→  + rail (left side, any row)
Arduino GND ──────────────→  − rail (left side, any row)
```

On the breadboard:

```
   +− rail
    ○ ← Jumper from Arduino 5V
    ○
    ○
    ○ ← Jumper from Arduino GND
    ○
```

Now every `+` hole on the left side has 5V and every `−` hole has GND. The right-side rails are also available if you need power on that side.

---

### Step 2: Potentiometer (Rows 1–3)

The potentiometer has 3 pins. Plug it into rows 1–3 on the left side (a–e):

```
    a  b  c  d  e
    ○  ○  ○  ○  ○  ← Row 1: Left pin → + rail (5V)
    ○  ○  ○  ○  ○  ← Row 2: Middle pin → Arduino A0
    ○  ○  ○  ○  ○  ← Row 3: Right pin → − rail (GND)
```

Wiring:

```
    + rail (5V)  ────→  Row 1 (any hole a1–e1)
    Arduino pin A0 ──→  Row 2 (any hole a2–e2)
    − rail (GND) ────→  Row 3 (any hole a3–e3)
```

---

### Step 3: Button (Rows 5–6)

Your button has **4 pins** in a square pattern. Internally, opposite corners are connected:

```
    Bottom view of the button:

    Pin 1 ○    ○ Pin 2

    Pin 3 ○    ○ Pin 4

    Internally:
    Pin 1 ↔ Pin 3 (left side, always connected)
    Pin 2 ↔ Pin 4 (right side, always connected)
    Press button → left connects to right
```

**It doesn't matter which way you orient the button.** No matter how you plug it in, the wiring is the same:

1. Place the button so it **straddles Row 5 and Row 6** (two pins in each row)
2. Connect Arduino pin **3** to **any hole in Row 5**
3. Connect GND to **any hole in Row 6**

When you press the button, Row 5 connects to Row 6 → circuit closes.

```
    a  b  c  d  e
    ○  ○  ○  ○  ○  ← Row 5: 2 button pins → Arduino pin 3
    ○  ○  ○  ○  ○  ← Row 6: 2 button pins → − rail (GND)
```

Wiring:

```
    Arduino pin 3  ────→  Row 5 (any hole a5–e5)
    − rail (GND)  ────→  Row 6 (any hole a6–e6)
```

---

### Step 4: MOSFET (Rows 8–10)

The IRLZ44N has 3 pins in a row. Plug it into rows 8–10. **Important:** The flat metal face should face toward the `a` side (left).

```
    a  b  c  d  e
    ○  ○  ○  ○  ○  ← Row 8:  Gate (pin 1) → Arduino pin 6
    ○  ○  ○  ○  ○  ← Row 9:  Drain (pin 2) → LED strip −
    ○  ○  ○  ○  ○  ← Row 10: Source (pin 3) → − rail (GND)
```

Wiring:

```
    Arduino pin 6  ────→  Row 8  (any hole a8–e8)
    LED strip −   ────→  Row 9  (any hole a9–e9)
    − rail (GND)  ────→  Row 10 (any hole a10–e10)
```

**Note:** The LED strip wires (24V circuit) should connect on the **right side** (f–j) of row 9, and the Arduino wires on the **left side** (a–e). This keeps high voltage separated from low voltage.

---

### Step 5: Motion Sensor (Off-board)

The HC-SR501 has pins spaced too wide for a breadboard row. Use male-to-female jumper wires:

```
    Motion Sensor           Breadboard
    ┌───────────┐
    │  ○ Fresnel│
    │    Lens   │          + rail (5V)
    │           │              ○
    └─┬───┬───┬─┘          − rail (GND)
      │   │   │                ○
     VCC OUT GND           Arduino pin 2
                              ○
    Wires:
    VCC ──→ + rail (5V)
    OUT ──→ Arduino pin 2
    GND ──→ − rail (GND)
```

Since the sensor is on flying leads (not plugged into the breadboard), just connect its wires to the appropriate rails and pin.

---

### Step 6: LM2596 (Off-board)

The LM2596 is a separate module. Use jumper wires:

```
    LM2596 Module
    ┌──────────────────────┐
    │  IN+  IN−  OUT+ OUT− │
    │   ●    ●    ●    ●  │
    └──────────────────────┘
         │     │     │     │
         │     │     │     └──→ − rail (GND)
         │     │     └────────→ + rail (5V)  [after adjusting to 5V!]
         │     └──────────────→ 24V Supply −
         └────────────────────→ 24V Supply +
```

**Before connecting OUT+ to the breadboard:** Adjust the trim pot until a multimeter reads 5.0V between OUT+ and OUT−.

---

### Step 7: LED Strip (Off-board)

The LED strip connects to the 24V circuit through the MOSFET:

```
    24V Supply +  ──────→  LED Strip + (red wire)
    LED Strip − (black)  ──→  Row 9, right side (f9–j9, MOSFET Drain)
    24V Supply −         ──→  − rail (GND)
```

---

## Complete Breadboard Map

Here's the full layout with all wires:

```
         ARDUINO UNO
    ┌─────────────────────────┐
    │ 6      3      2      A0 5V GND│
    │ ○      ○      ○      ○  ○  ○ │
    └──┼──────┼──────┼──────┼──┼──┼┘
       │     │     │     │   │  │
       │     │     │     │   │  │
       │     │     │     │   │  │
       │     │     │     │   │  │
    +− │     │     │     │   │  │ +−
     ○─┤     │     │     │   │  ├─○
     ○─┤     │     │     │   │  ├─○
     ○─┤     │     │     │   │  ├─○
     ○─┤     │     │     │   │  ├─○
     ○─┤     │     │     │   │  ├─○
     ○─┤     │     │     │   │  ├─○
     ○─┤ ┌───┼─────┼─────┼───┼──┤─○
     ○─┤ │a1 ○  b1 ○  c1 ○  d1 ○  e1│ │  f1 ○  g1 ○  h1 ○  i1 ○  j1 ○│ ├─○ Row 1  ← POT Left (5V)
     ○─┤ │a2 ○  b2 ○  c2 ○  d2 ○  e2│ │  f2 ○  g2 ○  h2 ○  i2 ○  j2 ○│ ├─○ Row 2  ← POT Middle (A0)
     ○─┤ │a3 ○  b3 ○  c3 ○  d3 ○  e3│ │  f3 ○  g3 ○  h3 ○  i3 ○  j3 ○│ ├─○ Row 3  ← POT Right (GND)
     ○─┤ │a4 ○  b4 ○  c4 ○  d4 ○  e4│ │  f4 ○  g4 ○  h4 ○  i4 ○  j4 ○│ ├─○ Row 4
     ○─┤ │a5 ○  b5 ○  c5 ○  d5 ○  e5│ │  f5 ○  g5 ○  h5 ○  i5 ○  j5 ○│ ├─○ Row 5  ← Button A (pin 3)
     ○─┤ │a6 ○  b6 ○  c6 ○  d6 ○  e6│ │  f6 ○  g6 ○  h6 ○  i6 ○  j6 ○│ ├─○ Row 6  ← Button B (GND)
     ○─┤ │a7 ○  b7 ○  c7 ○  d7 ○  e7│ │  f7 ○  g7 ○  h7 ○  i7 ○  j7 ○│ ├─○ Row 7
     ○─┤ │a8 ○  b8 ○  c8 ○  d8 ○  e8│ │  f8 ○  g8 ○  h8 ○  i8 ○  j8 ○│ ├─○ Row 8  ← MOSFET Gate (pin 6)
     ○─┤ │a9 ○  b9 ○  c9 ○  d9 ○  e9│ │  f9 ○  g9 ○  h9 ○  i9 ○  j9 ○│ ├─○ Row 9  ← MOSFET Drain (LED−)
     ○─┤ │a10○  b10○  c10○  d10○  e10│ │ f10○  g10○  h10○  i10○  j10○ │ ├─○ Row 10 ← MOSFET Source (GND)
     ○─┤ │a11○  b11○  c11○  d11○  e11│ │ f11○  g11○  h11○  i11○  j11○ │ ├─○ Row 11
     ○─┤ └───┼─────┼─────┼───┼──┘ ├─○
     ○─┤     │     │     │   │  ├─○
     ...    ...                       ...
     ○─┤     │     │     │   │  ├─○
     ○─┤     │     │     │   │  ├─○
    +− │     │     │     │   │  │ +−
       │     │     │     │   │  │
```

---

## Wire Summary Table

| From | To | Wire Color (suggested) |
|------|----|----------------------|
| Arduino 5V | + rail (left side) | Red |
| Arduino GND | − rail (left side) | Black |
| + rail | Pot left pin (row 1, left side) | Red |
| Arduino A0 | Pot middle pin (row 2, left side) | Yellow |
| − rail | Pot right pin (row 3, left side) | Black |
| Arduino pin 3 | Button leg A (row 5, left side) | White |
| − rail | Button leg B (row 6, left side) | Black |
| Arduino pin 6 | MOSFET Gate, row 8 (left side) | Green |
| LED strip − | MOSFET Drain, row 9 (right side) | Black (thick) |
| − rail | MOSFET Source, row 10 (left side) | Black |
| Motion sensor VCC | + rail | Red |
| Motion sensor OUT | Arduino pin 2 | White |
| Motion sensor GND | − rail | Black |
| LM2596 OUT+ | + rail | Red (after adjusting to 5V) |
| LM2596 OUT− | − rail | Black |
| 24V Supply + | LM2596 IN+ and LED strip + | Red (thick) |
| 24V Supply − | LM2596 IN− and − rail | Black (thick) |

---

## Common Mistakes

### 1. Wrong Row Alignment

Each pin must be in its own row. If two MOSFET pins share a row, they're shorted:

```
    WRONG (pins shorted):          RIGHT (each pin separate):
    a  ○  ○  ○  ○  ○  Row 8       a  ○  ○  ○  ○  ○  Row 8   ← Gate
    a  ○  ○  ○  ○  ○  Row 9       a  ○  ○  ○  ○  ○  Row 9   ← Drain
                                    a  ○  ○  ○  ○  ○  Row 10  ← Source
    Both pins in row 8 = BAD
```

### 2. Floating Wires

Every wire must go somewhere. A wire connected only on one side is "floating" and will pick up random noise:

```
    WRONG:                          RIGHT:
    Arduino pin 6 ──→ ○  (nowhere)  Arduino pin 6 ──→ ○ ← MOSFET Gate row
```

### 3. Power and Ground Short

If you accidentally connect + rail to − rail with a wire, you'll short the power supply:

```
    WRONG:
    + rail ○────────────○ − rail    ← SHORT CIRCUIT! (fire risk)
```

Always double-check before plugging in power.

### 4. Mixing High and Low Voltage

Keep 24V wires on the right side (f–j) and 5V wires on the left side (a–e):

```
    GOOD:                          BAD:
    Left (5V)  │  Right (24V)      5V and 24V wires
    a  b  c  d  e│f  g  h  i  j    mixed in same rows
    ○  ○  ○  ○  ○│○  ○  ○  ○  ○    ○ ○ 24V ○ ○
    ○  ○  ○  ○  ○│○  ○  ○  ○  ○    ○ ○  5V  ○ ○  ← RISK!
                 │
```

### 5. MOSFET Pin Order

The IRLZ44N pinout is Gate–Drain–Source (left to right when flat face faces you). Plugging it in backward will damage it:

```
    Flat face toward you:     Backward (wrong):
    ┌─────────┐               ┌─────────┐
    │  Metal  │               │  Metal  │
    └┬──┬──┬─┘               └┬──┬──┬─┘
     G  D  S                   S  D  G  ← WRONG!
```

---

## Tips

### Color Code Your Wires

| Color | Use |
|-------|-----|
| Red | 5V power |
| Black | GND |
| Yellow | Analog signals (potentiometer) |
| White | Digital inputs (motion sensor, button) |
| Green | Digital outputs (MOSFET gate) |
| Blue | Reserved for future use |

### Use Both Rail Sets

Your breadboard has power rails on both sides. Connect them together if you need 5V or GND available on both sides:

```
    Left + rail ────────○────── Right + rail
    Left − rail ────────○────── Right − rail
```

This gives you flexibility to grab power from whichever side is closer.

### Keep Wires Short

Long wires create clutter and can pick up noise. Route wires directly from point to point.

### One Wire Per Hole

Each hole can only hold one wire. If you need to split a signal (e.g., 5V to both pot and sensor), use the power rail as a distribution point.

### Using Bare Wire (Alambre)

If you're out of jumper wires, bare brass or copper wire works for short jumps:

- **Right gauge:** 22–24 AWG (0.5–0.6mm) — fits snugly without forcing
- **Keep wires separated** — bare wire touching = short circuit
- **Check before powering on** — use a multimeter in continuity mode between + and − rails to verify no shorts

---

## Testing on the Breadboard

Before connecting the 24V supply:

1. **Build the 5V side only** — potentiometer, button, motion sensor, MOSFET Gate
2. **Connect USB** to Arduino
3. **Open serial monitor** (`pio device monitor`)
4. **Test each input:**
   - Turn pot → `Pot:` value should change
   - Wave hand → `Motion:` should show `Y`
   - Press button → `Mode:` should toggle
5. **Check MOSFET Gate signal** — `Bright:` value should change as you turn the pot
6. **Only then** connect the 24V supply and LED strip

This staged approach catches wiring errors before they damage anything.
