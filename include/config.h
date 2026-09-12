#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Pin Definitions (ESP32 DevKit V1) ---
#define PIN_POTENTIOMETER  34   // Potentiometer wiper (ADC1, WiFi-safe)
#define PIN_BUTTON         27   // Mode toggle button (uses INPUT_PULLUP)
#define PIN_MOSFET         25   // IRLZ44N Gate — PWM output to LED strip
#define PIN_MODE_LED        2   // Built-in blue LED — indicates current mode

// --- LD2410C Radar Sensor (HardwareSerial2) ---
#define PIN_RADAR_RX       16   // LD2410C TX → ESP32 UART2 RX (direct, both 3.3V)
#define PIN_RADAR_TX       17   // LD2410C RX ← ESP32 UART2 TX (direct, both 3.3V)
#define RADAR_BAUD_RATE    115200  // LD2410C configured via Bluetooth app

// --- Thresholds ---
#define DEBOUNCE_MS        50   // Button debounce delay in milliseconds
#define PRESENCE_DEBOUNCE_MS  100  // Bidirectional: filter EMI glitches on loss and false spikes on detect (ms)
#define PRESENCE_COUNTDOWN_MS 15000 // Countdown before light turns off (ms)

// --- PWM ---
#define BRIGHTNESS_HYSTERESIS 3    // Min change to update PWM — filters ADC noise
#define FADE_MAX_MS         500UL  // Fade duration at full brightness (0-255). Scales with target.
#define MAX_BRIGHTNESS      255    // Full range — flicker fixed by PWM + write guard
#define PWM_FREQUENCY       1000   // 1 kHz — MOSFET fully switches, immune to WiFi clock jitter
#define PWM_RESOLUTION      8      // 8-bit resolution (0-255)
#define LEDC_CHANNEL_MOSFET 0     // Dedicated LEDC channel for MOSFET PWM
#define LEDC_TIMER          0     // Dedicated LEDC timer

// --- Modes ---
enum Mode {
  MODE_PRESENCE,  // Light triggered by presence sensor, pot sets max brightness
  MODE_MANUAL     // Pot directly controls brightness, presence ignored
};

Mode getMode();

bool getEffectivePresence();
bool getCountdownActive();
unsigned long getCountdownRemaining();

#endif
