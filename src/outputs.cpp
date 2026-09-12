#include "outputs.h"
#include "config.h"

static uint8_t lastBrightness = 0;
static bool lightIsOn = false;

void setupOutputs() {
  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(PIN_MODE_LED, OUTPUT);

  ledcSetup(LEDC_CHANNEL_MOSFET, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PIN_MOSFET, LEDC_CHANNEL_MOSFET);
  ledcWrite(LEDC_CHANNEL_MOSFET, 0);

  digitalWrite(PIN_MODE_LED, LOW);
}

void setBrightness(uint8_t value) {
  if (value == lastBrightness) return;
  ledcWrite(LEDC_CHANNEL_MOSFET, value);
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

void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
