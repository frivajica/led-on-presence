#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void mqttSetup();
void mqttLoop();
void mqttPublishAll(bool presence, int distanceCm, bool lightOn, uint8_t brightness,
                    int potValue);

#endif
