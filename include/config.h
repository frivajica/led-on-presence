#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Pin Definitions ---
#define PIN_POTENTIOMETER  A0   // Potentiometer wiper (analog input)
#define PIN_BUTTON         3    // Mode toggle button (digital input, uses INPUT_PULLUP)
#define PIN_MOSFET         6    // IRLZ44N Gate — PWM output to LED strip
#define PIN_MODE_LED       13   // Built-in LED — indicates current mode

// --- LD2410C Radar Sensor ---
#define PIN_RADAR_RX       10   // LD2410C TX → Arduino RX (SoftwareSerial)
#define PIN_RADAR_TX       11   // LD2410C RX ← Arduino TX (via voltage divider)
#define RADAR_BAUD_RATE    38400
#define RADAR_INIT_BAUD    256000
#define RADAR_MAX_GATE     5    // 5 × 75cm = 450cm ≈ 4m detection range
#define RADAR_NO_ONE_WINDOW 5   // seconds before sensor reports "no one"

// --- Thresholds ---
#define DEBOUNCE_MS        50   // Button debounce delay in milliseconds

// --- PWM ---
#define FADE_STEP          5    // Brightness change per loop iteration (0-255)

// --- Motion Timeout ---
#define MOTION_TIMEOUT_MS  15000UL  // 15 seconds after last motion before fade-out
#define COOLDOWN_MS        2000UL   // 2 seconds ignore sensor after fade-out (prevents re-trigger from LED heat)

// --- Modes ---
enum Mode {
  MODE_MOTION,   // Light triggered by motion sensor, pot sets max brightness
  MODE_MANUAL    // Pot directly controls brightness, motion ignored
};

#endif
