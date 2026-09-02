# LED-on-presence

Motion-activated 24V COB LED strip with smooth PWM dimming, controlled by an ESP32 and an LD2410C mmWave radar sensor. Connects to Home Assistant via MQTT.

## How It Works

- **Motion mode** (default): Radar detects presence → light fades up. After 15s with no presence + 2s cooldown → light fades out. Potentiometer sets max brightness.
- **Manual mode**: Potentiometer directly controls brightness (0–100%). Radar is ignored.
- **Button** toggles between modes. Built-in LED (GPIO 2) is ON in manual mode.
- **WiFi**: Connects to your network for wireless updates and Home Assistant integration.
- **Gas detection**: Steren ARD-352 sensor monitors air quality. Alarm threshold configurable from Home Assistant.
- **Temperature & humidity**: Steren ARD-360 (DHT11) monitors room conditions.
- **Web UI**: Live sensor dashboard at `http://<esp32-ip>`.

## Components

| Part | Qty | Purpose |
|------|-----|---------|
| ESP-WROOM-32 DevKit V1 | 1 | Brain (3.3V logic, hardware UART2, WiFi) |
| 24V COB LED Strip (Lumiora) | 1 | Light |
| LD2410C mmWave Radar Sensor | 1 | Presence detection (moving + stationary) |
| IRLZ44N MOSFET | 1 | PWM dimming of 24V LED strip |
| LM2596 Buck Converter | 1 | Steps 24V down to 5V for ESP32 |
| Steren ARD-352 Gas Sensor | 1 | Smoke and gas detection (MQ-2 based) |
| Steren ARD-360 Temp/Humidity Sensor | 1 | Room temperature and humidity (DHT11) |
| Potentiometer (10kΩ) | 1 | Brightness control |
| Momentary push button | 1 | Mode toggle |
| 24V DC Power Supply | 1 | Powers LED strip + ESP32 (via LM2596) |
| Breadboard (400 tie-points) | 1 | Prototyping platform |
| Jumper wires | ~16 | Connections |

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

# Upload via USB (first time)
~/.platformio/penv/bin/pio run -t upload

# Subsequent uploads — no USB needed (OTA)
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
| IRLZ44N MOSFET | Gate → GPIO 25, Source → GND, Drain → LED− |
| LD2410C | VCC → ESP32 3V3, TX → GPIO 16, RX → GPIO 17, GND → GND |
| Gas sensor (ARD-352) | VCC → 5V, GND → GND, DO → GPIO 14, AO → GPIO 32 |
| Temp/humidity (ARD-360) | VCC → 3V3, GND → GND, Data → GPIO 13 |
| LED strip | + → 24V+, − → MOSFET Drain |

**Important:** Gas sensor VCC must be 5V (heater requirement). All other sensors use 3.3V.

## Home Assistant Integration

### MQTT Auto-Discovery

Once WiFi + MQTT are configured, the device auto-registers in Home Assistant:

| Entity | Type | Description |
|--------|------|-------------|
| `light.led_on_presence` | Light | LED strip (brightness + on/off) |
| `binary_sensor.presence` | Binary sensor | Radar presence detection |
| `sensor.radar_distance` | Sensor | Detection distance in cm |
| `sensor.gas_level` | Sensor | Gas concentration (0-4095) |
| `binary_sensor.gas_detected` | Binary sensor | Gas alarm (ON/OFF) |
| `sensor.temperature` | Sensor | Room temperature (°C) |
| `sensor.humidity` | Sensor | Room humidity (%) |
| `sensor.brightness_pot` | Sensor | Potentiometer position |
| `number.gas_threshold` | Number | Gas alarm threshold (configurable) |

### MQTT Topics

```
led-on-presence/light/state          → {"state":"ON","brightness":128}
led-on-presence/light/set            → {"state":"ON","brightness":200}
led-on-presence/binary_sensor/presence/state → "ON" / "OFF"
led-on-presence/sensor/gas_level/state       → "350"
led-on-presence/sensor/temperature/state     → "23.5"
led-on-presence/sensor/humidity/state        → "45.2"
led-on-presence/config/gas_threshold/set     → "400"
```

### Web UI

Open `http://<esp32-ip>` in a browser to see live sensor values and toggle the light.

## Customization

All settings live in `include/config.h`.

### Change the fade speed

```cpp
#define FADE_STEP 5  // brightness change per loop (0-255)
```

### Change the motion timeout

```cpp
#define MOTION_TIMEOUT_MS 15000UL  // 15 seconds after last presence
```

### Change pins

```cpp
#define PIN_POTENTIOMETER  34   // ADC1, input-only
#define PIN_BUTTON         27   // uses INPUT_PULLUP
#define PIN_MOSFET         25   // must be PWM-capable
#define PIN_GAS_DIGITAL    14   // MQ-2 digital output
#define PIN_GAS_ANALOG     32   // MQ-2 analog output
#define PIN_DHT            13   // DHT11 data pin (ARD-360)
```

### Tune the radar sensor

The LD2410C is configured automatically on first boot and stores settings in flash. To reconfigure, change these constants and re-upload:

```cpp
#define RADAR_MAX_GATE       8    // 0–8, detect across full range (~6m)
#define RADAR_GATE_SENSITIVITY 10  // 0–100, lower = more sensitive (0 disables gate)
#define RADAR_IDLE_TIME      10   // seconds absent before "no one" reported
```

### Tune gas sensor threshold

The gas threshold is stored in flash and can be changed via MQTT from Home Assistant (recommended), or by editing `config.h`:

```cpp
#define GAS_THRESHOLD_DEFAULT 400  // 0-4095, lower = more sensitive
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
│   ├── gas_sensor.h / .cpp     # Steren ARD-352 gas sensor reading
│   ├── temperature_sensor.h / .cpp # Steren ARD-360 temp/humidity (DHT11)
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
WiFi: IP 192.168.1.50
MQTT: connected
Web: server started on port 80
Radar: config OK
LED-on-presence started
Mode: MOTION (default)
Pot: 512 Bright: 127/128 Pres: Y 85cm State: ACTIVE 15s Light: ON Gas: 120
```

## Docs

- [Arduino Basics](docs/arduino-basics.md) — pins, voltage, memory, web dev analogies
- [Breadboard Guide](docs/breadboard.md) — how breadboards work, visual layout for this project
- [Components](docs/components.md) — what each part does, how it works electrically
- [Wiring](docs/wiring.md) — step-by-step connection guide with ASCII diagrams
- [Code Walkthrough](docs/code-walkthrough.md) — line-by-line explanation of the code
