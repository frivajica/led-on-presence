#include "inputs.h"
#include "config.h"

static bool lastButtonState = HIGH;
static bool lastStableState = HIGH;
static unsigned long lastDebounceTime = 0;
static int lastPotValue = 0;
static float emaValue = -1;
static constexpr float EMA_ALPHA = 0.15;

void setupInputs() {
  analogReadResolution(10);
  pinMode(PIN_POTENTIOMETER, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

int readPotentiometer() {
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) {
    sum += analogRead(PIN_POTENTIOMETER);
  }
  int sample = sum / 8;

  if (emaValue < 0) {
    emaValue = sample;
  } else {
    emaValue = EMA_ALPHA * sample + (1.0 - EMA_ALPHA) * emaValue;
  }

  lastPotValue = (int)emaValue;
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
