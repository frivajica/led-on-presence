# LED-on-presence

Motion-activated 24V COB LED strip with smooth PWM dimming, controlled by an ESP32 and an LD2410C mmWave radar sensor.

## How It Works

- **Motion mode** (default): Radar detects presence → light fades up. After 15s with no presence + 2s cooldown → light fades out. Potentiometer sets max brightness.
- **Manual mode**: Potentiometer directly controls brightness (0–100%). Radar is ignored.
- **Button** toggles between modes. Built-in LED (GPIO 2) is ON in manual mode.

## Components

| Part | Qty | Purpose |
|------|-----|---------|
| ESP-WROOM-32 DevKit V1 | 1 | Brain (3.3V logic, hardware UART2) |
| 24V COB LED Strip (Lumiora) | 1 | Light |
| LD2410C mmWave Radar Sensor | 1 | Presence detection (moving + stationary) |
| IRLZ44N MOSFET | 1 | PWM dimming of 24V LED strip |
| LM2596 Buck Converter | 1 | Steps 24V down to 5V for ESP32 |
| Potentiometer (10kΩ) | 1 | Brightness control |
| Momentary push button | 1 | Mode toggle |
| 24V DC Power Supply | 1 | Powers LED strip + ESP32 (via LM2596) |
| Breadboard (400 tie-points) | 1 | Prototyping platform |
| Jumper wires | ~12 | Connections |

## Quick Start

```bash
# Build
~/.platformio/penv/bin/pio run

# Upload (connect ESP32 via USB first)
~/.platformio/penv/bin/pio run -t upload

# Monitor serial output
~/.platformio/penv/bin/pio device monitor
```

If `pio` is on your PATH, use `pio` directly instead of the full path.

## Wiring

See [docs/wiring.md](docs/wiring.md) for step-by-step connections with diagrams.

New to breadboards? Start with [docs/breadboard.md](docs/breadboard.md) — it explains how breadboards work and shows exactly where each component goes.

**TL;DR:**

| Component | Pins |
|-----------|------|
| LM2596 | 24V+ → IN+, 24V− → IN−, OUT+ → ESP32 VIN, OUT− → GND |
| Potentiometer | 3V3 → left pin, GPIO 34 → middle, GND → right |
| Button | GPIO 27 → GND (uses INPUT_PULLUP) |
| IRLZ44N MOSFET | Gate → GPIO 25, Source → GND, Drain → LED− |
| LD2410C | VCC → ESP32 3V3, TX → GPIO 16, RX → GPIO 17, GND → GND |
| LED strip | + → 24V+, − → MOSFET Drain |

**Important:** Disconnect USB when powering via LM2596. Do not use both simultaneously.

## Customization

All settings live in `include/config.h`.

### Change the fade speed

```cpp
#define FADE_STEP 5  // brightness change per loop (0-255)
```

- Higher value = faster fade (e.g., `15` for snappy)
- Lower value = slower fade (e.g., `1` for very smooth)

### Change the motion timeout

```cpp
#define MOTION_TIMEOUT_MS 15000UL  // 15 seconds after last presence
```

### Change pins

```cpp
#define PIN_POTENTIOMETER  34   // ADC1, input-only
#define PIN_BUTTON         27   // uses INPUT_PULLUP
#define PIN_MOSFET         25   // must be PWM-capable
```

### Tune the radar sensor

The LD2410C is configured automatically on first boot and stores settings in flash. To reconfigure, change these constants and re-upload:

```cpp
#define RADAR_MAX_GATE       8    // 0–8, detect across full range (~6m)
#define RADAR_GATE_SENSITIVITY 10  // 0–100, lower = more sensitive (0 disables gate)
#define RADAR_IDLE_TIME      10   // seconds absent before "no one" reported
```

## Project Structure

```
led-on-presence/
├── platformio.ini          # Build config (ESP32, ld2410 library)
├── include/
│   └── config.h            # Pin definitions and constants
├── src/
│   ├── main.cpp            # Entry point — setup, loop, state machine
│   ├── inputs.h / .cpp     # Read potentiometer and button
│   ├── outputs.h / .cpp    # Control MOSFET (PWM) and mode LED
│   └── radar.h / .cpp      # LD2410C radar communication and config
└── docs/
    ├── arduino-basics.md   # Arduino intro for web devs
    ├── breadboard.md       # Breadboard guide with visual layouts
    ├── components.md       # What each part does
    ├── wiring.md           # Connection guide with diagrams
    └── code-walkthrough.md # Line-by-line code explanation
```

## Serial Output

On boot, you'll see:

```
Radar: config OK
LED-on-presence started
Mode: MOTION (default)
Pot: 512 Bright: 127/128 Pres: Y 85cm State: ACTIVE 15s Light: ON
```

## Docs

- [Arduino Basics](docs/arduino-basics.md) — pins, voltage, memory, web dev analogies
- [Breadboard Guide](docs/breadboard.md) — how breadboards work, visual layout for this project
- [Components](docs/components.md) — what each part does, how it works electrically
- [Wiring](docs/wiring.md) — step-by-step connection guide with ASCII diagrams
- [Code Walkthrough](docs/code-walkthrough.md) — line-by-line explanation of the code
