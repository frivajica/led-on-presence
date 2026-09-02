#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <Arduino.h>

void setupOutputs();
void setBrightness(uint8_t value);
uint8_t getCurrentBrightness();
void setLightOn(bool on);
bool isLightOn();
void toggleLight();
void setModeLed(bool on);

#endif
