#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <Arduino.h>

void setupOutputs();
void setBrightness(uint8_t pin, uint8_t value);
void setModeLed(bool on);

#endif
