#include "radar.h"
#include "config.h"
#include <HardwareSerial.h>
#include <ld2410.h>

static ld2410 radar;
static bool _radarConnected = false;

static bool radarNeedsConfig() {
  if (radar.max_moving_gate != RADAR_MAX_GATE ||
      radar.max_stationary_gate != RADAR_MAX_GATE ||
      radar.sensor_idle_time != RADAR_IDLE_TIME) {
    return true;
  }
  for (uint8_t gate = 0; gate <= RADAR_MAX_GATE; gate++) {
    if (radar.motion_sensitivity[gate] != RADAR_GATE_SENSITIVITY ||
        radar.stationary_sensitivity[gate] != RADAR_GATE_SENSITIVITY) {
      return true;
    }
  }
  return false;
}

void setupRadar() {
  Serial2.setRxBufferSize(2048);
  Serial2.begin(RADAR_BAUD_RATE, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  delay(500);
  while (Serial2.available()) Serial2.read();

  _radarConnected = radar.begin(Serial2, false);

  // Defensive: recover a sensor left stuck in config mode (e.g. power lost
  // mid-configuration). Ignored by the sensor when already in data mode.
  static const byte LEAVE_CFG[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
  Serial2.write(LEAVE_CFG, sizeof(LEAVE_CFG));
  Serial2.flush();
  delay(100);
  while (Serial2.available()) Serial2.read();

  if (!radar.requestCurrentConfiguration()) {
    Serial.println(F("Radar: cfg query FAIL"));
    return;
  }

  if (!radarNeedsConfig()) {
    Serial.println(F("Radar: config OK"));
    return;
  }

  bool ok = true;
  for (uint8_t gate = 0; gate <= RADAR_MAX_GATE; gate++) {
    if (!radar.setGateSensitivityThreshold(gate, RADAR_GATE_SENSITIVITY, RADAR_GATE_SENSITIVITY)) ok = false;
  }
  if (!radar.setMaxValues(RADAR_MAX_GATE, RADAR_MAX_GATE, RADAR_IDLE_TIME)) ok = false;
  Serial.println(ok ? F("Radar: configured") : F("Radar: config FAIL"));
}

bool radarConnected() {
  return _radarConnected;
}

bool radarPresenceDetected() {
  if (!_radarConnected) return false;
  radar.read();
  return radar.presenceDetected();
}

bool radarMovingTargetDetected() {
  if (!_radarConnected) return false;
  return radar.movingTargetDetected();
}

bool radarStationaryTargetDetected() {
  if (!_radarConnected) return false;
  return radar.stationaryTargetDetected();
}

int radarDetectedDistance() {
  if (!_radarConnected) return 0;
  return radar.detectionDistance();
}
