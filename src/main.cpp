#include "config.h"
#include "inputs.h"
#include "outputs.h"
#include "radar.h"
#include "wifi_manager.h"
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

static int stablePotValue = 0;

// Software countdown: controls light off timing independently of sensor.
static unsigned long pendingSince = 0;
static unsigned long countdownStart = 0;
static bool pendingPresenceLost = false;
static bool countdownActive = false;
static bool countdownCompleted = false;
static bool effectivePresence = true;

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

static void printStatus(int potValue, bool radarPresence, bool effectivePresence, int target) {
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
  Serial.print(F(" Radar: "));
  Serial.print(radarPresence ? F("Y") : F("N"));
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
  Serial.print(F(" Fade: "));
  Serial.println(fading ? F("Y") : F("N"));
}

bool getEffectivePresence() { return effectivePresence; }
bool getCountdownActive() { return countdownActive; }
unsigned long getCountdownRemaining() {
  if (!countdownActive) return 0;
  return PRESENCE_COUNTDOWN_MS - (millis() - countdownStart);
}

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

  if (abs(potValue - stablePotValue) > 2) {
    stablePotValue = potValue;
  }

  // Software debounce + countdown for light-off timing.
  if (radarPresence) {
    pendingPresenceLost = false;
    countdownActive = false;
    countdownStart = 0;
    countdownCompleted = false;
    effectivePresence = true;
  } else {
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

  updatePresenceState(stablePotValue, effectivePresence);

  int target = isLightOn() ? map(stablePotValue, 0, 1023, 255, 0) : 0;
  if (target > MAX_BRIGHTNESS) target = MAX_BRIGHTNESS;

  if (effectivePresence != lastPresence) {
    bool directionChanged = (effectivePresence && targetBrightness == 0) ||
                            (!effectivePresence && targetBrightness > 0);
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

  printStatus(stablePotValue, radarPresence, effectivePresence, target);
}
