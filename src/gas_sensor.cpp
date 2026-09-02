#include "gas_sensor.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;
static uint16_t alarmThreshold = GAS_THRESHOLD_DEFAULT;

void setupGasSensor() {
  pinMode(PIN_GAS_DIGITAL, INPUT);
  prefs.begin("gas", false);
  uint16_t stored = prefs.getUShort("threshold", 0);
  if (stored > 0) {
    alarmThreshold = stored;
  }
}

bool gasReadDigital() {
  return digitalRead(PIN_GAS_DIGITAL) == LOW;
}

uint16_t gasReadAnalog() {
  return analogRead(PIN_GAS_ANALOG);
}

bool gasIsAlarm() {
  return gasReadAnalog() > alarmThreshold;
}

uint16_t gasGetThreshold() {
  return alarmThreshold;
}

void gasSetThreshold(uint16_t threshold) {
  alarmThreshold = threshold;
  prefs.putUShort("threshold", threshold);
}
