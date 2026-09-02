#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>

void setupTemperatureSensor();
bool temperaturePoll();

float temperatureGetCelsius();
float temperatureGetHumidity();
bool temperatureIsValid();

#endif
