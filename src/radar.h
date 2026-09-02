#ifndef RADAR_H
#define RADAR_H

void setupRadar();
bool radarConnected();
bool radarPresenceDetected();
bool radarMovingTargetDetected();
bool radarStationaryTargetDetected();
int radarDetectedDistance();

#endif
