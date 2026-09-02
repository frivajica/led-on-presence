#ifndef RADAR_H
#define RADAR_H

void setupRadar();
bool radarPresenceDetected();
bool radarMotionDetected();
bool radarIsConnected();
int radarDetectedDistance();

#endif
