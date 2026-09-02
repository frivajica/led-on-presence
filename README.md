# LED-on-presence

Motion-activated 24V COB LED strip with smooth PWM dimming, controlled by an Arduino Uno.

## How It Works

- **Motion mode** (default): PIR sensor detects movement → light fades up. After 30s with no motion → light fades out. Potentiometer sets max brightness.
- **Manual mode**: Potentiometer directly controls brightness (0–100%). Motion sensor is ignored.
- **Button** toggles between modes. Built-in LED (pin 13) is ON in manual mode.

## Components

| Part | Qty | Purpose |
|------|-----|---------|
| Arduino Uno R3 | 1 | Brain |
| 24V COB LED Strip (Lumiora) | 1 | Light |
| HC-SR501 PIR Motion Sensor | 1 | Motion detection |
| IRLZ44N MOSFET | 1 | PWM dimming of 24V LED strip |
| LM2596 Buck Converter | 1 | Steps 24V down to 5V for Arduino |
| Potentiometer (10kΩ) | 1 | Brightness control |
| Momentary push button | 1 | Mode toggle |
| 24V DC Power Supply | 1 | Powers LED strip + Arduino (via LM2596) |
| Breadboard (400 tie-points) | 1 | Prototyping platform |
| Jumper wires | ~12 | Connections |

## Quick Start

```bash
# Build
pio run

# Upload (connect Arduino via USB first)
pio run -t upload

# Monitor serial output
pio device monitor
```

If `pio` is not on your PATH:
```bash
~/.platformio/penv/bin/pio run
~/.platformio/penv/bin/pio run -t upload
~/.platformio/penv/bin/pio device monitor
```

## Wiring

See [docs/wiring.md](docs/wiring.md) for step-by-step connections with diagrams.

New to breadboards? Start with [docs/breadboard.md](docs/breadboard.md) — it explains how breadboards work and shows exactly where each component goes.

**TL;DR:**

| Component | Pins |
|-----------|------|
| LM2596 | 24V+ → IN+, 24V− → IN−, OUT+ → Arduino 5V, OUT− → GND |
| Potentiometer | 5V → A0 → GND |
| Motion sensor | 5V → D2 → GND |
| Button | D3 → GND (uses INPUT_PULLUP) |
| IRLZ44N MOSFET | Gate → D6, Source → GND, Drain → LED− |
| LED strip | + → 24V+, − → MOSFET Drain |

**Important:** Disconnect USB when powering via LM2596. Do not use both simultaneously.

## Customization

### Change the fade speed

Edit `FADE_STEP` in `include/config.h`:

```cpp
#define FADE_STEP 5  // brightness change per loop (0-255)
```

- Higher value = faster fade (e.g., `15` for snappy)
- Lower value = slower fade (e.g., `1` for very smooth)

### Change the motion timeout

Edit `MOTION_TIMEOUT_MS` in `include/config.h`:

```cpp
#define MOTOTION_TIMEOUT_MS 30000UL  // 30 seconds
```

### Change pins

Edit `include/config.h`:

```cpp
#define PIN_POTENTIOMETER  A0
#define PIN_MOTION_SENSOR  2
#define PIN_BUTTON         3
#define PIN_MOSFET         6   // Must be PWM pin (3, 5, 6, 9, 10, or 11)
```

### Adjust HC-SR501 sensor

- **Left pot (sensitivity):** Detection range, 3–7m. Set to ~50%.
- **Right pot (delay):** Set to minimum (fully CCW). The Arduino handles the 30s timeout.

## Project Structure

```
led-on-presence/
├── platformio.ini          # Build config
├── include/
│   └── config.h            # Pin definitions and constants
├── src/
│   ├── main.cpp            # Entry point — setup + loop
│   ├── inputs.h / .cpp     # Read sensors and button
│   └── outputs.h / .cpp    # Control MOSFET and LED
└── docs/
    ├── arduino-basics.md   # Arduino intro for web devs
    ├── breadboard.md       # Breadboard guide with visual layouts
    ├── components.md       # What each part does
    ├── wiring.md           # Connection guide with diagrams
    └── code-walkthrough.md # Line-by-line code explanation
```

## Docs

- [Arduino Basics](docs/arduino-basics.md) — pins, voltage, memory, web dev analogies
- [Breadboard Guide](docs/breadboard.md) — how breadboards work, visual layout for this project
- [Components](docs/components.md) — what each part does, how it works electrically
- [Wiring](docs/wiring.md) — step-by-step connection guide with ASCII diagrams
- [Code Walkthrough](docs/code-walkthrough.md) — line-by-line explanation of the code
