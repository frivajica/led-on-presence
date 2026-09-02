#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void mqttSetup();
void mqttLoop();
void mqttPublishPresence(bool detected, int distanceCm);
void mqttPublishLight(bool on, uint8_t brightness);
void mqttPublishGas(uint16_t level, bool alarm);
void mqttPublishPot(int value);

#endif
