# LED-on-presence

Presence-activated 24V COB LED strip with smooth PWM dimming, controlled by an ESP32 and an LD2410C mmWave radar sensor. Connects to Home Assistant via MQTT.

## How It Works

- **Presence mode** (default): Radar detects presence → light fades up. When presence is lost, light fades out. Potentiometer sets max brightness.
- **Manual mode**: Potentiometer directly controls brightness (0–100%). Radar is ignored.
- **Button** toggles between modes. Built-in LED (GPIO 2) is ON in manual mode.
- **WiFi**: Connects to your network for wireless updates and Home Assistant integration.
- **Web UI**: Live sensor dashboard at `http://<esp32-ip>`.

## Components

| Part | Qty | Purpose |
|------|-----|---------|
| ESP-WROOM-32 DevKit V1 | 1 | Brain (3.3V logic, hardware UART2, WiFi) |
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

### 1. Configure WiFi and MQTT

Edit `include/secrets.h` (gitignored):

```cpp
#define WIFI_SSID       "your-wifi-name"
#define WIFI_PASSWORD    "your-wifi-password"
#define MQTT_BROKER_IP   "192.168.1.100"
#define MQTT_BROKER_PORT 1883
```

### 2. Build and upload

```bash
# Build
~/.platformio/penv/bin/pio run

# Upload via USB
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
| IRLZ44N MOSFET | Gate → GPIO 23, Source → GND, Drain → LED− |
| LD2410C | VCC → ESP32 3V3, TX → GPIO 16, RX → GPIO 17, GND → GND |
| LED strip | + → 24V+, − → MOSFET Drain |

## Home Assistant Integration

### MQTT Auto-Discovery

Once WiFi + MQTT are configured, the device auto-registers in Home Assistant:

| Entity | Type | Description |
|--------|------|-------------|
| `light.led_on_presence` | Light | LED strip (brightness + on/off) |
| `binary_sensor.presence` | Binary sensor | Radar presence detection |
| `sensor.radar_distance` | Sensor | Detection distance in cm |
| `sensor.brightness_pot` | Sensor | Potentiometer position |

### MQTT Topics

```
led-on-presence/light/state          → {"state":"ON","brightness":128}
led-on-presence/light/set            → {"state":"ON","brightness":200}
led-on-presence/binary_sensor/presence/state → "ON" / "OFF"
led-on-presence/sensor/radar_distance/state  → "85"
led-on-presence/sensor/brightness_pot/state  → "512"
```

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
#define PIN_MOSFET         23   // PWM-capable
```

### Tune the radar sensor

The LD2410C is configured automatically on first boot and stores settings in flash. To reconfigure, change these constants and re-upload:

```cpp
#define RADAR_MAX_GATE                8    // 0–8, detect across full range (~6m)
#define RADAR_MOTION_SENSITIVITY      15   // 0–100, lower = more sensitive
#define RADAR_STATIONARY_SENSITIVITY  15   // 0–100, lower = more sensitive
#define RADAR_IDLE_TIME               15   // Seconds absent before "no one" reported
```

## Project Structure

```
led-on-presence/
├── platformio.ini              # Build config + libraries
├── include/
│   ├── config.h                # Pin definitions and constants
│   └── secrets.h               # WiFi/MQTT credentials (gitignored)
├── src/
│   ├── main.cpp                # Entry point — setup, loop, state machine
│   ├── inputs.h / .cpp         # Read potentiometer and button
│   ├── outputs.h / .cpp        # Control MOSFET (PWM), mode LED, light state
│   ├── radar.h / .cpp          # LD2410C radar communication and config
│   ├── wifi_manager.h / .cpp   # WiFi connect + auto-reconnect
│   ├── mqtt_handler.h / .cpp   # MQTT + Home Assistant auto-discovery
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
MQTT: connected
Web: server started on port 80
Radar: config OK
LED-on-presence started
Mode: PRESENCE (default)
Loop: 500/s
Pot: 512 Bright: 127/128 Pres: Y 85cm State: ACTIVE Light: ON
```

## Docs

- [Arduino Basics](docs/arduino-basics.md) — pins, voltage, memory, web dev analogies
- [Breadboard Guide](docs/breadboard.md) — how breadboards work, visual layout for this project
- [Components](docs/components.md) — what each part does, how it works electrically
- [Wiring](docs/wiring.md) — step-by-step connection guide with ASCII diagrams
- [Code Walkthrough](docs/code-walkthrough.md) — line-by-line explanation of the code
