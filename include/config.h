#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Pin Definitions (ESP32 DevKit V1) ---
#define PIN_POTENTIOMETER  34   // Potentiometer wiper (ADC1, input-only)
#define PIN_BUTTON         27   // Mode toggle button (digital input, uses INPUT_PULLUP)
#define PIN_MOSFET         25   // IRLZ44N Gate — PWM output to LED strip
#define PIN_MODE_LED        2   // Built-in blue LED — indicates current mode

// --- LD2410C Radar Sensor (HardwareSerial2) ---
#define PIN_RADAR_RX       16   // LD2410C TX → ESP32 UART2 RX (direct, both 3.3V)
#define PIN_RADAR_TX       17   // LD2410C RX ← ESP32 UART2 TX (direct, both 3.3V)
#define RADAR_BAUD_RATE    256000  // LD2410C factory default
#define RADAR_MAX_GATE     8    // Detect across full range (~6m)
#define RADAR_GATE_SENSITIVITY 40  // Energy threshold per gate (lower = more sensitive; 0 disables)
#define RADAR_IDLE_TIME    10   // Seconds target must be absent before "no one" reported

// --- Gas Sensor (Steren ARD-352 / MQ-2) ---
#define PIN_GAS_DIGITAL    14   // Digital output: LOW = gas detected
#define PIN_GAS_ANALOG     32   // Analog output: voltage proportional to concentration
#define GAS_THRESHOLD_DEFAULT 400  // Default alarm threshold (0-4095); overridable via MQTT

// --- Temperature / Humidity Sensor (Steren ARD-360 / DHT11) ---
#define PIN_DHT            13   // DHT11 data pin
#define DHT_READ_INTERVAL  10000UL  // DHT11 is slow — re-read at most every 10s

// --- Thresholds ---
#define DEBOUNCE_MS        50   // Button debounce delay in milliseconds

// --- PWM ---
#define FADE_STEPS_PER_SEC  510UL  // Fade step density (0 = instant). 510 ≈ smooth fade over full 0-255 range.

// --- Modes ---
enum Mode {
  MODE_MOTION,   // Light triggered by motion sensor, pot sets max brightness
  MODE_MANUAL    // Pot directly controls brightness, motion ignored
};

Mode getMode();

#endif
