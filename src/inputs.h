#ifndef INPUTS_H
#define INPUTS_H

#include <Arduino.h>

void setupInputs();
int  readPotentiometer();
bool readMotion();
bool readButton();

#endif
