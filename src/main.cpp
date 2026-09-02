#include "config.h"
#include "inputs.h"
#include "outputs.h"
#include "radar.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "gas_sensor.h"
#include "temperature_sensor.h"
#include "web_server.h"
#include <ArduinoOTA.h>

enum PresenceState {
  PRESENCE_IDLE,
  PRESENCE_ACTIVE
};

static Mode currentMode = MODE_PRESENCE;
static PresenceState presenceState = PRESENCE_IDLE;
static uint8_t currentBrightness = 0;
static uint8_t targetBrightness = 0;
static bool lastPresence = false;

static uint8_t fadeStartBrightness = 0;
static unsigned long fadeStartTime = 0;
static bool fading = false;

Mode getMode() {
  return currentMode;
}

static void fadeStart(uint8_t target) {
  if (target == currentBrightness) return;
  fadeStartBrightness = currentBrightness;
  targetBrightness = target;
  fadeStartTime = millis();
  fading = true;
}

static void fadeUpdate() {
  if (!fading) return;
  unsigned long duration = (unsigned long)FADE_MAX_MS * targetBrightness / 255;
  if (duration < 50) duration = 50;
  unsigned long elapsed = millis() - fadeStartTime;
  if (elapsed >= duration) {
    currentBrightness = targetBrightness;
    fading = false;
  } else {
    float progress = (float)elapsed / duration;
    currentBrightness = fadeStartBrightness +
        (int)((int)targetBrightness - (int)fadeStartBrightness) * progress;
  }
}

static void updatePresenceState(int potValue, bool presence) {
  if (currentMode != MODE_PRESENCE) {
    setLightOn(potValue > 5);
    presenceState = PRESENCE_IDLE;
    return;
  }
  if (presence) {
    presenceState = PRESENCE_ACTIVE;
    setLightOn(true);
  } else {
    presenceState = PRESENCE_IDLE;
    setLightOn(false);
  }
}

static void printStatus(int potValue, bool presence) {
  static unsigned long lastPrint = 0;
  static unsigned long loopCounter = 0;
  static unsigned long lastLoopCount = 0;
  loopCounter++;
  if (millis() - lastLoopCount >= 1000) {
    Serial.print(F("Loop: "));
    Serial.print(loopCounter);
    Serial.println(F("/s"));
    loopCounter = 0;
    lastLoopCount = millis();
  }
  if (millis() - lastPrint <= 500) return;
  lastPrint = millis();

  Serial.print(F("Pot: "));
  Serial.print(potValue);
  Serial.print(F(" Bright: "));
  Serial.print(currentBrightness);
  Serial.print(F("/"));
  Serial.print(targetBrightness);
  Serial.print(F(" Pres: "));
  Serial.print(presence ? F("Y") : F("N"));
  if (presence) {
    Serial.print(F(" "));
    Serial.print(radarDetectedDistance());
    Serial.print(F("cm"));
  }
  Serial.print(F(" State: "));
  switch (presenceState) {
    case PRESENCE_IDLE:
      Serial.print(F("IDLE"));
      break;
    case PRESENCE_ACTIVE:
      Serial.print(F("ACTIVE"));
      break;
  }
  Serial.print(F(" Light: "));
  Serial.print(isLightOn() ? F("ON") : F("OFF"));
  uint16_t gasLevel = gasReadAnalog();
  Serial.print(F(" Gas: "));
  Serial.print(gasLevel);
  if (gasIsAlarm()) Serial.print(F(" ALARM"));
  if (temperatureIsValid()) {
    Serial.print(F(" Temp: "));
    Serial.print(temperatureGetCelsius(), 1);
    Serial.print(F("C Hum: "));
    Serial.print(temperatureGetHumidity(), 1);
    Serial.print(F("%"));
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  setupInputs();
  setupOutputs();
  setupRadar();
  setupGasSensor();
  setupTemperatureSensor();

  wifiSetup();
  mqttSetup();
  webServerSetup();

  ArduinoOTA.setHostname("led-on-presence");
  ArduinoOTA.begin();

  Serial.println(F("LED-on-presence started"));
  Serial.println(F("Mode: PRESENCE (default)"));
}

void loop() {
  // PRIORITY 1: LED control — never blocked by network
  int potValue = readPotentiometer();
  bool presence = radarPresenceDetected();

  updatePresenceState(potValue, presence);

  int target = isLightOn() ? map(potValue, 0, 1023, 255, 0) : 0;

  if (presence != lastPresence) {
    fadeStart(target);
  } else if ((int)target != (int)targetBrightness) {
    if (fading) {
      targetBrightness = target;
    } else {
      currentBrightness = target;
      targetBrightness = target;
    }
  }

  fadeUpdate();
  lastPresence = presence;

  setBrightness(currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  // PRIORITY 2: Inputs (fast, non-blocking)
  if (readButton()) {
    currentMode = (currentMode == MODE_PRESENCE) ? MODE_MANUAL : MODE_PRESENCE;
    lastPresence = presence;
    Serial.print(F("Mode: "));
    Serial.println(currentMode == MODE_PRESENCE ? F("PRESENCE") : F("MANUAL"));
  }

  // PRIORITY 3: Network — can block, runs after LED is updated
  ArduinoOTA.handle();
  wifiLoop();
  mqttLoop();
  temperaturePoll();

  // PRIORITY 4: Reporting — periodic, runs last
  static unsigned long lastMqttPublish = 0;
  if (millis() - lastMqttPublish > 2000) {
    lastMqttPublish = millis();
    uint16_t gasLevel = gasReadAnalog();
    mqttPublishAll(presence, radarDetectedDistance(), isLightOn(), currentBrightness,
                   gasLevel, gasIsAlarm(), potValue,
                   temperatureGetCelsius(), temperatureGetHumidity());
  }

  printStatus(potValue, presence);
}
