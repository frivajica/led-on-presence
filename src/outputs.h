#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <Arduino.h>

void setupOutputs();
void setBrightness(uint16_t value);
uint16_t getCurrentBrightness();
void setLightOn(bool on);
bool isLightOn();
void setModeLed(bool on);

#endif
