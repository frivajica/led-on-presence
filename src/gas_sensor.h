#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <Arduino.h>

void setupGasSensor();
bool gasReadDigital();
uint16_t gasReadAnalog();
bool gasIsAlarm();
uint16_t gasGetThreshold();
void gasSetThreshold(uint16_t threshold);

#endif
