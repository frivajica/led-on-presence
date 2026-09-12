#include "outputs.h"
#include "config.h"

static uint16_t lastBrightness = 0;
static bool lightIsOn = false;

void setupOutputs() {
  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(PIN_MODE_LED, OUTPUT);

  ledcAttach(PIN_MOSFET, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcWrite(PIN_MOSFET, 0);

  digitalWrite(PIN_MODE_LED, LOW);
}

void setBrightness(uint16_t value) {
  if (value == lastBrightness) return;
  ledcWrite(PIN_MOSFET, value);
  lastBrightness = value;
}

uint16_t getCurrentBrightness() {
  return lastBrightness;
}

void setLightOn(bool on) {
  lightIsOn = on;
}

bool isLightOn() {
  return lightIsOn;
}

void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
