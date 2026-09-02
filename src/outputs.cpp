#include "outputs.h"
#include "config.h"

static uint8_t lastBrightness = 0;
static bool lightIsOn = false;

void setupOutputs() {
  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(PIN_MODE_LED, OUTPUT);

  analogWrite(PIN_MOSFET, 0);
  digitalWrite(PIN_MODE_LED, LOW);
}

void setBrightness(uint8_t value) {
  analogWrite(PIN_MOSFET, value);
  lastBrightness = value;
}

uint8_t getCurrentBrightness() {
  return lastBrightness;
}

void setLightOn(bool on) {
  lightIsOn = on;
}

bool isLightOn() {
  return lightIsOn;
}

void toggleLight() {
  lightIsOn = !lightIsOn;
}

void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
