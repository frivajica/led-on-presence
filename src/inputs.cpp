#include "inputs.h"
#include "config.h"

static bool lastButtonState = HIGH;
static bool lastStableState = HIGH;
static unsigned long lastDebounceTime = 0;
static int lastPotValue = 0;

void setupInputs() {
  analogReadResolution(10);  // ESP32 defaults to 12-bit; use 10-bit to match Arduino Uno range (0-1023)
  pinMode(PIN_POTENTIOMETER, INPUT);
  // Button uses internal pull-up: pin reads HIGH when open, LOW when pressed
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

// Average 8 samples to reduce ADC electrical noise. Each sample is fast
// (~10 µs), so 8 reads take < 100 µs — no perceptible delay.
int readPotentiometer() {
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) {
    sum += analogRead(PIN_POTENTIOMETER);
  }
  lastPotValue = sum / 8;
  return lastPotValue;
}

int getPotValue() {
  return lastPotValue;
}

bool readButton() {
  bool reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  lastButtonState = reading;

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != lastStableState) {
      lastStableState = reading;
      // Return true only on press (HIGH → LOW transition)
      return reading == LOW;
    }
  }

  return false;
}
