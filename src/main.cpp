#include "config.h"
#include "inputs.h"
#include "outputs.h"
#include "radar.h"
#include "wifi_manager.h"
#include "web_server.h"
#include <ArduinoOTA.h>
#include <WiFi.h>

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

static int stablePotValue = 0;

// Software countdown: controls light off timing independently of sensor.
static unsigned long pendingSince = 0;
static unsigned long countdownStart = 0;
static bool pendingPresenceLost = false;
static bool countdownActive = false;
static bool countdownCompleted = false;
static bool effectivePresence = false;
static bool pendingPresenceDetected = false;
static unsigned long pendingDetectSince = 0;
static bool lastRadarConnected = false;
static unsigned long radarOfflineSince = 0;
static unsigned long radarOnlineSince = 0;
static bool sensorStabilizing = false;
static unsigned int disconnectCount = 0;
static unsigned long lastDisconnectTime = 0;

// Sensor offline grace period: freeze state for this duration before failing safe
static constexpr unsigned long SENSOR_OFFLINE_GRACE_MS = 2000;
// Sensor reconnect stabilization: wait this long before trusting presence data
static constexpr unsigned long SENSOR_STABILIZE_MS = 500;

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
  bool potEnabled = potValue < 1008;

  if (currentMode != MODE_PRESENCE) {
    setLightOn(potEnabled);
    presenceState = PRESENCE_IDLE;
    return;
  }

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

static void printStatus(int potValue, bool radarPresence, bool radarConnected, bool effectivePresence, int target) {
  static unsigned long lastPrint = 0;
  static unsigned long loopCounter = 0;
  static unsigned long lastLoopCount = 0;
  loopCounter++;
  if (millis() - lastLoopCount >= 1000) {
  Serial.print(F("Loop: "));
  Serial.print(loopCounter);
  Serial.print(F("/s WiFi: "));
  Serial.println(WiFi.status() == WL_CONNECTED ? F("OK") : F("DISC"));
    loopCounter = 0;
    lastLoopCount = millis();
  }
  if (millis() - lastPrint <= 500) return;
  lastPrint = millis();

  Serial.print(F(" Pot: "));
  Serial.print(potValue);
  Serial.print(F(" Bright: "));
  Serial.print(currentBrightness);
  Serial.print(F("/"));
  Serial.print(targetBrightness);
  Serial.print(F(" Radar: "));
  Serial.print(radarPresence ? F("Y") : F("N"));
  Serial.print(F(" RCon: "));
  Serial.print(radarConnected ? F("Y") : F("N"));
  Serial.print(F(" Eff: "));
  Serial.print(effectivePresence ? F("Y") : F("N"));
  Serial.print(F(" Countdown: "));
  if (countdownActive) {
    unsigned long remaining = PRESENCE_COUNTDOWN_MS - (millis() - countdownStart);
    Serial.print(remaining);
    Serial.print(F("ms"));
  } else {
    Serial.print(F("-"));
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
  Serial.print(F(" Tgt: "));
  Serial.print(target);
  Serial.print(F(" Dist: "));
  if (radarPresence) {
    Serial.print(radarDetectedDistance());
    Serial.print(F("cm"));
  } else {
    Serial.print(F("-"));
  }
  Serial.print(F(" Fade: "));
  Serial.print(fading ? F("Y") : F("N"));
  Serial.print(F(" Stab: "));
  Serial.print(sensorStabilizing ? F("Y") : F("N"));
  Serial.print(F(" Disc: "));
  Serial.println(disconnectCount);
}

bool getEffectivePresence() { return effectivePresence; }
bool getCountdownActive() { return countdownActive; }
unsigned long getCountdownRemaining() {
  if (!countdownActive) return 0;
  return PRESENCE_COUNTDOWN_MS - (millis() - countdownStart);
}
unsigned int getDisconnectCount() { return disconnectCount; }
unsigned long getLastDisconnectTime() { return lastDisconnectTime; }
bool isSensorStabilizing() { return sensorStabilizing; }

void setup() {
  Serial.begin(115200);
  setupInputs();
  setupOutputs();
  setupRadar();

  stablePotValue = readPotentiometer();

  wifiSetup();
  webServerSetup();

  ArduinoOTA.setHostname("led-on-presence");
  ArduinoOTA.begin();

  Serial.println(F("LED-on-presence started"));
  Serial.println(F("Mode: PRESENCE (default)"));
}

void loop() {
  int potValue = readPotentiometer();
  bool radarPresence = radarPresenceDetected();
  bool radarConnected = radarIsConnected();

  // Track sensor connect/disconnect events
  if (radarConnected != lastRadarConnected) {
    if (radarConnected) {
      radarOnlineSince = millis();
      sensorStabilizing = true;
      disconnectCount++;
      Serial.print(millis());
      Serial.print(F(" RADAR ONLINE ("));
      Serial.print(F("disconnects: "));
      Serial.print(disconnectCount);
      if (lastDisconnectTime > 0) {
        unsigned long offlineDuration = radarOnlineSince - lastDisconnectTime;
        Serial.print(F(", was offline for "));
        Serial.print(offlineDuration);
        Serial.print(F("ms"));
      }
      Serial.println(F(")"));
    } else {
      radarOfflineSince = millis();
      lastDisconnectTime = millis();
      sensorStabilizing = false;
      Serial.print(millis());
      Serial.println(F(" RADAR OFFLINE"));
    }
    lastRadarConnected = radarConnected;
  }

  if (abs(potValue - stablePotValue) > 2) {
    stablePotValue = potValue;
  }

  // State machine: freeze state when sensor is offline, resume normal after stabilization
  if (!radarConnected) {
    // Sensor offline: freeze effectivePresence and all state
    // Only invalidate incomplete detection — preserve absence/countdown tracking
    pendingPresenceDetected = false;
    // effectivePresence, pendingPresenceLost, countdownActive, countdownCompleted all stay as-is
  } else if (sensorStabilizing) {
    // Sensor just came online: wait for stabilization period before trusting data
    if (millis() - radarOnlineSince >= SENSOR_STABILIZE_MS) {
      sensorStabilizing = false;
      // Resume normal state machine with preserved state — don't force-set anything
    }
    // During stabilization, keep effectivePresence frozen
  } else {
    // Normal operation: sensor online and stabilized
    if (radarPresence) {
      pendingPresenceLost = false;
      countdownActive = false;
      countdownStart = 0;
      countdownCompleted = false;

      if (!effectivePresence) {
        if (!pendingPresenceDetected) {
          pendingPresenceDetected = true;
          pendingDetectSince = millis();
        } else if (millis() - pendingDetectSince >= PRESENCE_DEBOUNCE_MS) {
          effectivePresence = true;
        }
      }
    } else {
      pendingPresenceDetected = false;

      if (!pendingPresenceLost) {
        pendingPresenceLost = true;
        pendingSince = millis();
        countdownCompleted = false;
        effectivePresence = true;
      } else if (millis() - pendingSince >= PRESENCE_DEBOUNCE_MS) {
        if (!countdownActive && !countdownCompleted) {
          countdownActive = true;
          countdownStart = millis();
        }
        unsigned long elapsed = millis() - countdownStart;
        if (elapsed >= PRESENCE_COUNTDOWN_MS) {
          effectivePresence = false;
          countdownActive = false;
          countdownCompleted = true;
        } else {
          effectivePresence = true;
        }
      } else {
        effectivePresence = true;
      }
    }
  }

  updatePresenceState(stablePotValue, effectivePresence);

  int target = isLightOn() ? map(stablePotValue, 0, 1023, 255, 0) : 0;
  if (target > MAX_BRIGHTNESS) target = MAX_BRIGHTNESS;

  if (effectivePresence != lastPresence) {
    bool directionChanged = (effectivePresence && targetBrightness == 0) ||
                            (!effectivePresence && targetBrightness > 0);
    if (directionChanged) {
      fadeStart(target);
    }
  } else if (abs((int)target - (int)targetBrightness) >= BRIGHTNESS_HYSTERESIS) {
    if (fading) {
      targetBrightness = target;
    } else {
      currentBrightness = target;
      targetBrightness = target;
    }
  }

  fadeUpdate();
  lastPresence = effectivePresence;

  setBrightness(currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  if (readButton()) {
    currentMode = (currentMode == MODE_PRESENCE) ? MODE_MANUAL : MODE_PRESENCE;
    Serial.print(F("Mode: "));
    Serial.println(currentMode == MODE_PRESENCE ? F("PRESENCE") : F("MANUAL"));
  }

  ArduinoOTA.handle();
  wifiLoop();

  printStatus(stablePotValue, radarPresence, radarConnected, effectivePresence, target);
}
