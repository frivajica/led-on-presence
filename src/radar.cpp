#include "radar.h"
#include "config.h"
#include <SoftwareSerial.h>
#include <MyLD2410.h>

static SoftwareSerial radarSerial(PIN_RADAR_RX, PIN_RADAR_TX);
static MyLD2410 radar(radarSerial);

void setupRadar() {
  Serial.println(F("Radar: initializing at 256000..."));

  radarSerial.begin(RADAR_INIT_BAUD);
  delay(500);

  if (radar.begin()) {
    Serial.println(F("Radar: handshake OK at 256000"));

    radar.configMode(true);
    // setBaud takes an index: 1=9600, 2=19200, 3=38400, 4=57600, 5=115200, 6=230400, 7=256000
    if (radar.setBaud(3)) {
      Serial.println(F("Radar: baud set to 38400"));
    }
    radar.configMode(false);
    delay(100);
  } else {
    Serial.println(F("Radar: handshake FAIL at 256000, retrying at 38400..."));
  }

  radarSerial.begin(RADAR_BAUD_RATE);
  delay(500);

  if (!radar.begin()) {
    Serial.println(F("Radar: FAILED to connect"));
    while (true) {}
  }

  Serial.println(F("Radar: connected at 38400"));

  radar.configMode(true);
  radar.setMaxGate(RADAR_MAX_GATE, RADAR_MAX_GATE, RADAR_NO_ONE_WINDOW);
  radar.configMode(false);

  Serial.print(F("Radar: max gate "));
  Serial.print(RADAR_MAX_GATE);
  Serial.print(F(" ("));
  Serial.print((RADAR_MAX_GATE + 1) * radar.getResolution());
  Serial.println(F(" cm)"));
}

bool radarPresenceDetected() {
  radar.check();
  return radar.presenceDetected();
}

bool radarMovingTargetDetected() {
  return radar.movingTargetDetected();
}

bool radarStationaryTargetDetected() {
  return radar.stationaryTargetDetected();
}

int radarDetectedDistance() {
  return radar.detectedDistance();
}
