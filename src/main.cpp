#include "config.h"
#include "inputs.h"
#include "outputs.h"
#include "radar.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "gas_sensor.h"
#include "web_server.h"
#include <ArduinoOTA.h>

enum MotionState {
  MOTION_IDLE,
  MOTION_ACTIVE,
  MOTION_COOLDOWN
};

static Mode currentMode = MODE_MOTION;
static MotionState motionState = MOTION_IDLE;
static uint8_t currentBrightness = 0;
static uint8_t targetBrightness = 0;
static unsigned long lastMotionTime = 0;
static unsigned long cooldownStartTime = 0;

static void updateMotionState(int potValue, bool presence) {
  if (currentMode != MODE_MOTION) {
    setLightOn(potValue > 5);
    motionState = MOTION_IDLE;
    return;
  }
  switch (motionState) {
    case MOTION_IDLE:
      if (presence) {
        motionState = MOTION_ACTIVE;
        setLightOn(true);
        lastMotionTime = millis();
      }
      break;
    case MOTION_ACTIVE:
      if (presence) {
        lastMotionTime = millis();
      }
      if (millis() - lastMotionTime > MOTION_TIMEOUT_MS) {
        motionState = MOTION_COOLDOWN;
        cooldownStartTime = millis();
        setLightOn(false);
      }
      break;
    case MOTION_COOLDOWN:
      if (millis() - cooldownStartTime > COOLDOWN_MS) {
        motionState = MOTION_IDLE;
      }
      break;
  }
}

static void fadeToward(uint8_t target) {
  targetBrightness = target;
  if (currentBrightness < targetBrightness) {
    currentBrightness = min((int)(currentBrightness + FADE_STEP), (int)targetBrightness);
  } else if (currentBrightness > targetBrightness) {
    currentBrightness = max((int)(currentBrightness - FADE_STEP), (int)targetBrightness);
  }
}

static void printStatus(int potValue, bool presence) {
  static unsigned long lastPrint = 0;
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
  switch (motionState) {
    case MOTION_IDLE:
      Serial.print(F("IDLE"));
      break;
    case MOTION_ACTIVE: {
      unsigned long elapsed = millis() - lastMotionTime;
      unsigned long remaining = (elapsed < MOTION_TIMEOUT_MS) ? (MOTION_TIMEOUT_MS - elapsed) / 1000 : 0;
      Serial.print(F("ACTIVE "));
      Serial.print(remaining);
      Serial.print(F("s"));
      break;
    }
    case MOTION_COOLDOWN: {
      unsigned long elapsed = millis() - cooldownStartTime;
      unsigned long remaining = (elapsed < COOLDOWN_MS) ? (COOLDOWN_MS - elapsed) / 1000 : 0;
      Serial.print(F("COOL "));
      Serial.print(remaining);
      Serial.print(F("s"));
      break;
    }
  }
  Serial.print(F(" Light: "));
  Serial.print(isLightOn() ? F("ON") : F("OFF"));
  uint16_t gasLevel = gasReadAnalog();
  Serial.print(F(" Gas: "));
  Serial.print(gasLevel);
  if (gasIsAlarm()) Serial.print(F(" ALARM"));
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  setupInputs();
  setupOutputs();
  setupRadar();
  setupGasSensor();

  wifiSetup();
  mqttSetup();
  webServerSetup();

  ArduinoOTA.setHostname("led-on-presence");
  ArduinoOTA.begin();

  Serial.println(F("LED-on-presence started"));
  Serial.println(F("Mode: MOTION (default)"));
}

void loop() {
  ArduinoOTA.handle();
  wifiLoop();
  mqttLoop();

  if (readButton()) {
    currentMode = (currentMode == MODE_MOTION) ? MODE_MANUAL : MODE_MOTION;
    Serial.print(F("Mode: "));
    Serial.println(currentMode == MODE_MOTION ? F("MOTION") : F("MANUAL"));
  }

  int potValue = readPotentiometer();
  bool presence = radarPresenceDetected();

  updateMotionState(potValue, presence);

  if (isLightOn()) {
    fadeToward(map(potValue, 0, 1023, 255, 0));
  } else {
    fadeToward(0);
  }

  setBrightness(currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  static unsigned long lastMqttPublish = 0;
  if (millis() - lastMqttPublish > 2000) {
    lastMqttPublish = millis();
    mqttPublishPresence(presence, radarDetectedDistance());
    mqttPublishLight(isLightOn(), currentBrightness);
    mqttPublishGas(gasReadAnalog(), gasIsAlarm());
    mqttPublishPot(potValue);
  }

  printStatus(potValue, presence);
  delay(20);
}
