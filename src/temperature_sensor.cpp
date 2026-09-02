#include "temperature_sensor.h"
#include "config.h"
#include <DHT.h>

static DHT dht(PIN_DHT, DHT11);

static float lastTemp = NAN;
static float lastHumidity = NAN;
static unsigned long lastReadTime = 0;

void setupTemperatureSensor() {
  dht.begin();
  Serial.println(F("Temp: DHT11 initialized"));
}

bool temperaturePoll() {
  unsigned long now = millis();
  if (now - lastReadTime < DHT_READ_INTERVAL) return true;
  lastReadTime = now;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Temp: read failed"));
    return false;
  }

  lastTemp = t;
  lastHumidity = h;
  return true;
}

float temperatureGetCelsius() {
  return lastTemp;
}

float temperatureGetHumidity() {
  return lastHumidity;
}

bool temperatureIsValid() {
  return !isnan(lastTemp) && !isnan(lastHumidity);
}
