#include "config.h"
#include "inputs.h"
#include "outputs.h"
#include "radar.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
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

// Presence hysteresis: ignore rapid toggles from electrical noise.
// The radar can glitch when the MOSFET switches high current (4m LED
// strip). Require 400ms of stable presence before reacting.
static bool rawPresence = false;
static bool stablePresence = false;
static unsigned long presenceStableSince = 0;
static const unsigned long PRESENCE_HYSTERESIS_MS = 400;

// Potentiometer hysteresis: only accept pot changes > 2 counts.
// Cheap pots have ADC noise (~2 counts); this prevents brightness jitter
// at the off end where map() hovers between 0 and 6.
static int stablePotValue = 0;

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
  // Pot deadzone: last ~15 counts at the dim end (CW on inverted pot) = OFF.
  // Cheap pots rarely reach 1023; 1008 covers the typical 1010-1015 max.
  bool potEnabled = potValue < 1008;

  if (currentMode != MODE_PRESENCE) {
    setLightOn(potEnabled);
    presenceState = PRESENCE_IDLE;
    return;
  }

  // In presence mode, the pot is the master brightness control. If it's in
  // the off deadzone, stay off even when presence is detected.
  if (!potEnabled) {
    presenceState = PRESENCE_IDLE;
    setLightOn(false);
  } else if (presence) {
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
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  setupInputs();
  setupOutputs();
  setupRadar();

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
  rawPresence = radarPresenceDetected();

  // Potentiometer hysteresis: ignore changes smaller than 3 counts to
  // eliminate ADC noise jitter at the off end.
  if (abs(potValue - stablePotValue) > 2) {
    stablePotValue = potValue;
  }

  // Hysteresis filter: only accept presence changes after 400ms of stability.
  // This prevents EMI from the 4m LED strip from causing rapid toggles.
  if (rawPresence != stablePresence) {
    presenceStableSince = millis();
    stablePresence = rawPresence;
  }
  bool presence = stablePresence;
  if (millis() - presenceStableSince < PRESENCE_HYSTERESIS_MS) {
    presence = lastPresence;  // not stable yet, keep previous state
  }

  updatePresenceState(stablePotValue, presence);

  int target = isLightOn() ? map(stablePotValue, 0, 1023, 255, 0) : 0;
  if (target < 15) target = 0;  // Hard off below visible threshold

  // Only restart fade when presence direction actually changes.
  // If the radar glitches (bounces ON→OFF→ON within 400ms) while an
  // existing fade is already going the right way, don't restart it.
  if (presence != lastPresence) {
    bool directionChanged = (presence && targetBrightness == 0) ||
                            (!presence && targetBrightness > 0);
    if (directionChanged) {
      fadeStart(target);
    }
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

  // PRIORITY 4: Reporting — periodic, runs last
  static unsigned long lastMqttPublish = 0;
  if (millis() - lastMqttPublish > 2000) {
    lastMqttPublish = millis();
    mqttPublishAll(presence, radarDetectedDistance(), isLightOn(), currentBrightness,
                   stablePotValue);
  }

  printStatus(stablePotValue, rawPresence);
}
