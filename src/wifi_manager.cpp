#include "wifi_manager.h"
#include "config.h"
#include "secrets.h"
#include <WiFi.h>

static unsigned long lastReconnectAttempt = 0;
static bool connected = false;

void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("WiFi: connecting"));
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
    Serial.println(F(" OK"));
    Serial.print(F("WiFi: IP "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F(" timeout — continuing without WiFi"));
  }
}

void wifiLoop() {
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
    return;
  }
  connected = false;
  if (millis() - lastReconnectAttempt > 5000) {
    lastReconnectAttempt = millis();
    WiFi.reconnect();
  }
}

bool wifiIsConnected() {
  return connected;
}
