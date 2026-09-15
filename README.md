# LED-on-presence

Presence-activated 24V COB LED strip with smooth PWM dimming, controlled by an ESP32 and an LD2410C mmWave radar sensor. Connects to WiFi for a web dashboard and Arduino OTA updates.

## How It Works

- **Presence mode** (default): Radar detects presence → light fades up. When presence is lost, a 15-second countdown begins before the light fades out. Potentiometer sets max brightness.
- **Manual mode**: Potentiometer directly controls brightness (0–100%). Radar is ignored.
- **Button** toggles between modes. Built-in LED (GPIO 2) is ON in manual mode.
- **WiFi**: Connects to your network for the web UI and OTA updates.
- **Web UI**: Live sensor dashboard at `http://<esp32-ip>`.
- **Arduino OTA**: Wireless firmware updates over WiFi.

### Signal Processing

The radar signal passes through three software layers before reaching the output:

1. **EMI filter**: Presence reports with 0cm distance are rejected (buck converter noise can corrupt UART frames into ghost presence reports)
2. **Bidirectional debounce**: A detection must persist for 100ms before activating; a loss must persist for 100ms before the countdown starts
3. **Software countdown**: After the radar reports absence + 100ms debounce, a 15-second countdown begins. The light stays on during this period.

The potentiometer signal also passes through noise filters:

1. **8-sample averaging** per read to reduce ADC noise
2. **Exponential Moving Average (EMA)** with α=0.15 to smooth across reads
3. **Hysteresis** (±3 units): the brightness target only updates when the change exceeds 3/255

## Components

| Part | Qty | Purpose |
|------|-----|---------|
| ESP-WROOM-32 DevKit V1 | 1 | Brain (3.3V logic, hardware UART2, WiFi) |
| 24V COB LED Strip (Lumiora) | 1 | Light |
| LD2410C mmWave Radar Sensor | 1 | Presence detection (moving + stationary) |
| IRLZ44N MOSFET | 1 | PWM dimming of 24V LED strip |
| 220Ω resistor | 1 | Series gate resistor for MOSFET (reduces gate ringing) |
| LM2596 Buck Converter | 1 | Steps 24V down to 5V for ESP32 |
| Potentiometer (10kΩ) | 1 | Brightness control |
| Momentary push button | 1 | Mode toggle |
| 24V DC Power Supply | 1 | Powers LED strip + ESP32 (via LM2596) |
| Breadboard (400 tie-points) | 1 | Prototyping platform |
| Protoboard (perfboard) | 1 | Permanent soldered assembly |
| Jumper wires | ~12 | Prototype connections |
| Hookup wire (22 AWG) | ~1 m | Protoboard connections |

## Quick Start

### 1. Configure WiFi

Edit `include/secrets.h` (gitignored):

```cpp
#define WIFI_SSID       "your-wifi-name"
#define WIFI_PASSWORD    "your-wifi-password"
```

### 2. Build and upload

```bash
# Build
~/.platformio/penv/bin/pio run
```

Choose one upload method in `platformio.ini`:

**USB serial** (first flash or if WiFi is unavailable):
```ini
upload_protocol = esptool
upload_port = /dev/cu.usbserial-0001
```

**WiFi OTA** (convenient after initial setup):
```ini
upload_protocol = espota
upload_port = 192.168.1.203
```

Then upload:
```bash
~/.platformio/penv/bin/pio run -t upload

# Monitor serial output
~/.platformio/penv/bin/pio device monitor
```

If `pio` is on your PATH, use `pio` directly.

## Wiring

See [docs/wiring.md](docs/wiring.md) for step-by-step connections with diagrams.

**TL;DR:**

| Component | Pins |
|-----------|------|
| LM2596 | 24V+ → IN+, 24V− → IN−, OUT+ → ESP32 VIN, OUT− → GND |
| Potentiometer | 3V3 → left pin, GPIO 34 → middle, GND → right |
| Button | GPIO 27 → GND (uses INPUT_PULLUP) |
| IRLZ44N MOSFET | Gate → 220Ω → GPIO 25, Source → GND, Drain → LED− |
| LD2410C | VCC → ESP32 3V3, TX → GPIO 16, RX → GPIO 17, GND → GND |
| LED strip | + → 24V+, − → MOSFET Drain |

### Web UI

Open `http://<esp32-ip>` in a browser to see live sensor values.

## Customization

All settings live in `include/config.h`.

### Change the fade duration

```cpp
#define FADE_MAX_MS  500UL  // Fade duration at full brightness (0-255). Scales with target.
```

### Change pins

```cpp
#define PIN_POTENTIOMETER  34   // ADC1, input-only, WiFi-safe
#define PIN_BUTTON         27   // Uses INPUT_PULLUP
#define PIN_MOSFET         25   // PWM-capable (SigmaDelta)
```

### Tune the radar sensor

The LD2410C is configured via the **HLKRadarTool** Bluetooth app (iOS/Android):

1. Connect to the sensor via Bluetooth
2. Adjust sensitivity per distance gate (gates 0–8)
3. Set the "unmanned" (no-one) duration — 0 seconds is recommended so the sensor reports absence immediately
4. Changes are saved to the sensor's flash and persist across power cycles

### Change thresholds

```cpp
#define PRESENCE_DEBOUNCE_MS  100   // ms to filter EMI glitches on detect and loss
#define PRESENCE_COUNTDOWN_MS 15000 // ms to keep light on after absence confirmed
#define BRIGHTNESS_HYSTERESIS 3     // min brightness change to update PWM (8-bit)
#define PWM_FREQUENCY         2000000 // 2 MHz SigmaDelta carrier frequency
```

## Project Structure

```
led-on-presence/
├── platformio.ini              # Build config + libraries
├── include/
│   ├── config.h                # Pin definitions and constants
│   └── secrets.h               # WiFi credentials (gitignored)
├── src/
│   ├── main.cpp                # Entry point — setup, loop, state machine
│   ├── inputs.h / .cpp         # Read potentiometer (EMA-filtered) and button
│   ├── outputs.h / .cpp        # Control MOSFET (SigmaDelta PWM), mode LED
│   ├── radar.h / .cpp          # LD2410C radar (autoReadTask, EMI filter)
│   ├── wifi_manager.h / .cpp   # WiFi connect + auto-reconnect
│   └── web_server.h / .cpp     # Minimal web UI for debugging
└── docs/
    ├── arduino-basics.md       # Arduino intro for web devs
    ├── breadboard.md           # Breadboard guide with visual layouts
    ├── components.md           # What each part does
    ├── wiring.md               # Connection guide with diagrams
    └── code-walkthrough.md     # Line-by-line code explanation
```

## Serial Output

On boot:

```
WiFi: connecting.... OK
WiFi: IP 192.168.1.203
Web: server started on port 80
Radar: firmware v1.0.0
LED-on-presence started
Mode: PRESENCE (default)
Loop: 500/s WiFi: OK
 Pot: 512 Bright: 127/128 Radar: Y RCon: Y Eff: Y Countdown: - State: ACTIVE Light: ON Tgt: 128 Dist: 150cm Fade: N
```

**Serial fields:**

| Field | Meaning |
|-------|---------|
| `Pot` | Filtered potentiometer value (0–1023) |
| `Bright` | current / target PWM value (0–255) |
| `Radar` | Y/N — radar reports presence (after EMI filter) |
| `RCon` | Y/N — radar UART connection state |
| `Eff` | Y/N — effective presence (after debounce + countdown) |
| `Countdown` | ms remaining, or `-` when inactive |
| `State` | IDLE or ACTIVE |
| `Light` | ON or OFF |
| `Tgt` | Target brightness value |
| `Dist` | Detection distance in cm, or `-` |
| `Fade` | Y/N — fade in progress |

`RADAR OFFLINE` / `RADAR ONLINE` events are printed with timestamps when the UART connection state changes.

## Troubleshooting

### Radar not responding (`Radar: no response to firmware query`)

Some LD2410C units ship with **UART disabled** (Bluetooth-only mode). If the ESP32 can't communicate with the sensor:

1. Download **HLKRadarTool** app (iOS/Android)
2. Connect to the sensor via Bluetooth
3. Go to **Settings** → **Restore Factory**
4. Reboot the ESP32

The factory reset re-enables UART at 115200 baud.

### Light turns on unexpectedly (phantom trigger)

The buck converter's switching noise can corrupt UART frames into ghost presence reports. The software EMI filter rejects these (distance = 0cm is impossible for real targets). If phantom triggers persist:

- Move the buck converter away from the ESP32 and UART wires (inverse square law — 10cm separation eliminates ~90% of interference)
- Route UART wires away from 24V power wires

### LED flickering (fast micro-stutter)

Caused by the IRLZ44N operating at the edge of its linear region with only 3.3V gate drive. Fixed by:

- Using a 220Ω series resistor between GPIO 25 and the MOSFET gate (not to ground)
- The 2 MHz SigmaDelta PWM signal is naturally smoothed by the MOSFET's gate capacitance + 220Ω resistor

### Radar goes offline frequently

UART EMI from the buck converter. The LD2410C broadcasts frames continuously; brief corruption causes the library to miss a frame and report "offline." This is cosmetic — the software reconnects within 100ms and the debounce layer filters out noise. To reduce offline events, increase physical separation between the buck converter and UART wires.

## Moving to Protoboard

Once the breadboard prototype is verified:

1. **Transfer the circuit** to a protoboard (perfboard) using the same connections
2. **Solder all joints** — no more loose jumper wires
3. **Use 22 AWG solid-core wire** for power rails and signal lines
4. **Keep the ESP32 socketed** or use female headers so you can remove it for updates
5. **Test each section** as you solder: power first, then outputs, then inputs, then radar

See [Wiring](docs/wiring.md) for the exact pin assignments.

## Docs

- [Arduino Basics](docs/arduino-basics.md) — pins, voltage, memory, web dev analogies
- [Breadboard Guide](docs/breadboard.md) — how breadboards work, visual layout for this project
- [Components](docs/components.md) — what each part does, how it works electrically
- [Wiring](docs/wiring.md) — step-by-step connection guide with ASCII diagrams
- [Code Walkthrough](docs/code-walkthrough.md) — line-by-line explanation of the code
