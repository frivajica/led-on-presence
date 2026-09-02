#include "radar.h"
#include "config.h"
#include <HardwareSerial.h>
#include <ld2410.h>

static ld2410 radar;
static bool sensorReady = false;

// LD2410 command: leave configuration mode (restore data mode).
// Sent defensively at boot in case a previous power loss left the sensor stuck
// in config mode. Ignored when already in data mode.
static const byte CMD_LEAVE_CONFIG[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};

static bool radarNeedsConfig() {
  if (radar.max_moving_gate != RADAR_MAX_GATE ||
      radar.max_stationary_gate != RADAR_MAX_GATE ||
      radar.sensor_idle_time != RADAR_IDLE_TIME) {
    return true;
  }
  for (uint8_t gate = 0; gate <= RADAR_MAX_GATE; gate++) {
    if (radar.motion_sensitivity[gate] != RADAR_MOTION_SENSITIVITY ||
        radar.stationary_sensitivity[gate] != RADAR_STATIONARY_SENSITIVITY) {
      return true;
    }
  }
  return false;
}

// Configure the LD2410C sensor on first boot (or if config changed).
// The sensor stores config in flash, so subsequent boots skip this — fast
// boot and no unnecessary flash wear.
void setupRadar() {
  Serial2.setRxBufferSize(2048);
  Serial2.begin(RADAR_BAUD_RATE, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  delay(500);
  while (Serial2.available()) Serial2.read();

  sensorReady = radar.begin(Serial2, false);

  // Defensive: recover a sensor left stuck in config mode (e.g. power lost
  // mid-configuration). Ignored by the sensor when already in data mode.
  Serial2.write(CMD_LEAVE_CONFIG, sizeof(CMD_LEAVE_CONFIG));
  Serial2.flush();
  delay(100);
  while (Serial2.available()) Serial2.read();

  if (!radar.requestCurrentConfiguration()) {
    Serial.println(F("Radar: cfg query FAIL"));
    return;
  }

  if (!radarNeedsConfig()) {
    Serial.println(F("Radar: config OK"));
    radar.autoReadTask();
    return;
  }

  bool ok = true;
  for (uint8_t gate = 0; gate <= RADAR_MAX_GATE; gate++) {
    if (!radar.setGateSensitivityThreshold(gate, RADAR_MOTION_SENSITIVITY, RADAR_STATIONARY_SENSITIVITY)) ok = false;
  }
  if (!radar.setMaxValues(RADAR_MAX_GATE, RADAR_MAX_GATE, RADAR_IDLE_TIME)) ok = false;
  Serial.println(ok ? F("Radar: configured") : F("Radar: config FAIL"));
  if (ok) radar.autoReadTask();
}

bool radarPresenceDetected() {
  if (!sensorReady || !radar.isConnected()) return false;
  return radar.presenceDetected();
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
