#include "inputs.h"
#include "config.h"

static bool lastButtonState = HIGH;
static bool lastStableState = HIGH;
static unsigned long lastDebounceTime = 0;

void setupInputs() {
  pinMode(PIN_POTENTIOMETER, INPUT);
  pinMode(PIN_MOTION_SENSOR, INPUT);
  // Button uses internal pull-up: pin reads HIGH when open, LOW when pressed
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

int readPotentiometer() {
  return analogRead(PIN_POTENTIOMETER);
}

bool readMotion() {
  return digitalRead(PIN_MOTION_SENSOR) == HIGH;
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
