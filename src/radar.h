#ifndef RADAR_H
#define RADAR_H

#include <Arduino.h>

void setupRadar();
bool radarPresenceDetected();
bool radarMovingTargetDetected();
bool radarStationaryTargetDetected();
int  radarDetectedDistance();

#endif
