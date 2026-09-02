#include "config.h"
#include "inputs.h"
#include "outputs.h"

enum MotionState {
  MOTION_IDLE,
  MOTION_ACTIVE,
  MOTION_COOLDOWN
};

static Mode currentMode = MODE_MOTION;
static MotionState motionState = MOTION_IDLE;
static uint8_t currentBrightness = 0;
static uint8_t targetBrightness = 0;
static bool lightOn = false;
static unsigned long lastMotionTime = 0;
static unsigned long cooldownStartTime = 0;

void setup() {
  Serial.begin(9600);
  setupInputs();
  setupOutputs();

  Serial.println(F("LED-on-presence started"));
  Serial.println(F("Mode: MOTION (default)"));
}

void loop() {
  // --- Button: toggle motion enabled/disabled ---
  if (readButton()) {
    currentMode = (currentMode == MODE_MOTION) ? MODE_MANUAL : MODE_MOTION;

    Serial.print(F("Mode: "));
    Serial.println(currentMode == MODE_MOTION ? F("MOTION") : F("MANUAL"));
  }

  // --- Read inputs ---
  int potValue = readPotentiometer();
  bool motionDetected = readMotion();

  // --- Motion state machine ---
  if (currentMode == MODE_MOTION) {
    switch (motionState) {
      case MOTION_IDLE:
        if (motionDetected) {
          motionState = MOTION_ACTIVE;
          lightOn = true;
          lastMotionTime = millis();
        }
        break;

      case MOTION_ACTIVE:
        if (motionDetected) {
          lastMotionTime = millis();
        }
        if (millis() - lastMotionTime > MOTION_TIMEOUT_MS) {
          motionState = MOTION_COOLDOWN;
          cooldownStartTime = millis();
          lightOn = false;
        }
        break;

      case MOTION_COOLDOWN:
        if (millis() - cooldownStartTime > COOLDOWN_MS) {
          motionState = MOTION_IDLE;
        }
        break;
    }
  } else {
    // Manual mode: potentiometer directly controls on/off
    lightOn = potValue > 5;
    motionState = MOTION_IDLE;
  }

  // --- Set target brightness ---
  if (lightOn) {
    targetBrightness = map(potValue, 0, 1023, 255, 0);
  } else {
    targetBrightness = 0;
  }

  // --- Fade current brightness toward target ---
  if (currentBrightness < targetBrightness) {
    currentBrightness = min(currentBrightness + FADE_STEP, targetBrightness);
  } else if (currentBrightness > targetBrightness) {
    currentBrightness = max(currentBrightness - FADE_STEP, targetBrightness);
  }

  // --- Apply brightness ---
  setBrightness(PIN_MOSFET, currentBrightness);
  setModeLed(currentMode == MODE_MANUAL);

  // --- Debug output ---
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.print(F("Pot: "));
    Serial.print(potValue);
    Serial.print(F(" Bright: "));
    Serial.print(currentBrightness);
    Serial.print(F("/"));
    Serial.print(targetBrightness);
    Serial.print(F(" Motion: "));
    Serial.print(motionDetected ? F("Y") : F("N"));
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
    Serial.println(lightOn ? F("ON") : F("OFF"));
  }

  delay(20);
}
