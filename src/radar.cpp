#include "radar.h"
#include "config.h"
#include <HardwareSerial.h>
#include <ld2410.h>

static ld2410 radar;
static bool sensorReady = false;

void setupRadar() {
  Serial2.setRxBufferSize(8192);
  Serial2.begin(RADAR_BAUD_RATE, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  delay(500);
  while (Serial2.available()) Serial2.read();

  sensorReady = radar.begin(Serial2, true);

  if (!sensorReady) {
    Serial.println(F("Radar: no response to firmware query — check wiring/baud rate"));
    radar.autoReadTask(4096, 3, 1);
    return;
  }

  Serial.print(F("Radar: firmware v"));
  Serial.print(radar.firmware_major_version);
  Serial.print('.');
  Serial.print(radar.firmware_minor_version);
  Serial.print('.');
  Serial.println(radar.firmware_bugfix_version, HEX);

  radar.autoReadTask(4096, 3, 1);
}

bool radarPresenceDetected() {
  if (!sensorReady || !radar.isConnected()) return false;
  if (!radar.presenceDetected()) return false;
  // EMI noise can report presence with 0 distance — filter it out
  return radar.detectionDistance() > 0;
}

bool radarMotionDetected() {
  if (!sensorReady || !radar.isConnected()) return false;
  return radar.movingTargetDetected();
}

bool radarIsConnected() {
  if (!sensorReady) return false;
  return radar.isConnected();
}

int radarDetectedDistance() {
  if (!sensorReady || !radar.isConnected()) return 0;
  return radar.detectionDistance();
}
