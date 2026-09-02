#include "mqtt_handler.h"
#include "config.h"
#include "secrets.h"
#include "gas_sensor.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static bool discoverySent = false;

static String topicFor(const char* suffix) {
  return String(MQTT_DEVICE_NAME) + "/" + suffix;
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  String thresholdTopic = topicFor("config/gas_threshold/set");
  if (String(topic) == thresholdTopic) {
    uint16_t val = msg.toInt();
    if (val > 0 && val <= 4095) {
      gasSetThreshold(val);
      Serial.print(F("MQTT: gas threshold set to "));
      Serial.println(val);
    }
  }
}

static void sendDiscovery() {
  String base = "homeassistant/";
  String dev = "\"dev\":[\"ids\":[\"" + String(MQTT_DEVICE_NAME) + "\"],\"name\":\"LED on Presence\",\"mf\":\"DIY\"]";

  const char* binarySensors[][2] = {
    {"binary_sensor/presence", "Presence"},
    {"binary_sensor/gas_detected", "Gas Detected"},
  };
  for (auto& s : binarySensors) {
    String topic = base + s[0] + "/config";
    String payload = "{\"name\":\"" + String(s[1]) + "\",\"state_topic\":\"" + topicFor(s[0]) + "/state\"," + dev + "}";
    mqtt.publish(topic.c_str(), payload.c_str(), true);
  }

  const char* sensors[][2] = {
    {"sensor/gas_level", "Gas Level"},
    {"sensor/brightness_pot", "Brightness Pot"},
    {"sensor/radar_distance", "Radar Distance"},
    {"sensor/temperature", "Temperature"},
    {"sensor/humidity", "Humidity"},
  };
  for (auto& s : sensors) {
    String topic = base + s[0] + "/config";
    String payload = "{\"name\":\"" + String(s[1]) + "\",\"state_topic\":\"" + topicFor(s[0]) + "/state\"," + dev + "}";
    mqtt.publish(topic.c_str(), payload.c_str(), true);
  }

  String lightTopic = base + "light/strip/config";
  String lightPayload = "{\"name\":\"LED Strip\",\"state_topic\":\"" + topicFor("light/state") + "\",\"command_topic\":\"" + topicFor("light/set") + "\",\"brightness\":true,\"brightness_scale\":255," + dev + "}";
  mqtt.publish(lightTopic.c_str(), lightPayload.c_str(), true);

  String thresholdTopic = base + "number/gas_threshold/config";
  String thresholdPayload = "{\"name\":\"Gas Threshold\",\"state_topic\":\"" + topicFor("config/gas_threshold/state") + "\",\"command_topic\":\"" + topicFor("config/gas_threshold/set") + "\",\"min\":0,\"max\":4095," + dev + "}";
  mqtt.publish(thresholdTopic.c_str(), thresholdPayload.c_str(), true);

  String currentThreshold = String(gasGetThreshold());
  mqtt.publish(topicFor("config/gas_threshold/state").c_str(), currentThreshold.c_str(), true);

  discoverySent = true;
}

void mqttSetup() {
  mqtt.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);
}

void mqttLoop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqtt.connected()) {
    unsigned long now = millis();
    static unsigned long lastAttempt = 0;
    if (now - lastAttempt > 5000) {
      lastAttempt = now;
      if (mqtt.connect(MQTT_DEVICE_NAME)) {
        Serial.println(F("MQTT: connected"));
        discoverySent = false;
        mqtt.subscribe(topicFor("config/gas_threshold/set").c_str());
      }
    }
    return;
  }

  mqtt.loop();

  if (!discoverySent) {
    sendDiscovery();
  }
}

void mqttPublishPresence(bool detected, int distanceCm) {
  if (!mqtt.connected()) return;
  mqtt.publish(topicFor("binary_sensor/presence/state").c_str(), detected ? "ON" : "OFF", true);
  mqtt.publish(topicFor("sensor/radar_distance/state").c_str(), String(distanceCm).c_str(), true);
}

void mqttPublishLight(bool on, uint8_t brightness) {
  if (!mqtt.connected()) return;
  JsonDocument doc;
  doc["state"] = on ? "ON" : "OFF";
  doc["brightness"] = brightness;
  char buf[64];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(topicFor("light/state").c_str(), buf, true);
}

void mqttPublishGas(uint16_t level, bool alarm) {
  if (!mqtt.connected()) return;
  mqtt.publish(topicFor("sensor/gas_level/state").c_str(), String(level).c_str(), true);
  mqtt.publish(topicFor("binary_sensor/gas_detected/state").c_str(), alarm ? "ON" : "OFF", true);
}

void mqttPublishPot(int value) {
  if (!mqtt.connected()) return;
  mqtt.publish(topicFor("sensor/brightness_pot/state").c_str(), String(value).c_str(), true);
}

void mqttPublishTemperature(float celsius, float humidity) {
  if (!mqtt.connected()) return;
  mqtt.publish(topicFor("sensor/temperature/state").c_str(), String(celsius, 1).c_str(), true);
  mqtt.publish(topicFor("sensor/humidity/state").c_str(), String(humidity, 1).c_str(), true);
}
