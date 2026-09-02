#include "outputs.h"
#include "config.h"

void setupOutputs() {
  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(PIN_MODE_LED, OUTPUT);

  analogWrite(PIN_MOSFET, 0);
  digitalWrite(PIN_MODE_LED, LOW);
}

void setBrightness(uint8_t pin, uint8_t value) {
  analogWrite(pin, value);
}

void setModeLed(bool on) {
  digitalWrite(PIN_MODE_LED, on ? HIGH : LOW);
}
