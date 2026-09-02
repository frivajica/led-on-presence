#include "mqtt_handler.h"
#include "config.h"
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static bool discoverySent = false;
static bool mqttConnecting = false;

static String topicFor(const char* suffix) {
  return String(MQTT_DEVICE_NAME) + "/" + suffix;
}

static void mqttConnectTask(void* param) {
  for (;;) {
    if (mqtt.connected() || WiFi.status() != WL_CONNECTED) {
      mqttConnecting = false;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    mqttConnecting = true;
    if (mqtt.connect(MQTT_DEVICE_NAME)) {
      Serial.println(F("MQTT: connected"));
      discoverySent = false;
    }
    mqttConnecting = false;
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

static void sendDiscovery() {
  String base = "homeassistant/";
  String dev = "\"dev\":[\"ids\":[\"" + String(MQTT_DEVICE_NAME) + "\"],\"name\":\"LED Multisensor\",\"mf\":\"DIY\"]";

  const char* binarySensors[][2] = {
    {"binary_sensor/presence", "Presence"},
  };
  for (auto& s : binarySensors) {
    String topic = base + s[0] + "/config";
    String payload = "{\"name\":\"" + String(s[1]) + "\",\"state_topic\":\"" + topicFor(s[0]) + "/state\"," + dev + "}";
    mqtt.publish(topic.c_str(), payload.c_str(), true);
  }

  const char* sensors[][2] = {
    {"sensor/brightness_pot", "Brightness Pot"},
    {"sensor/radar_distance", "Radar Distance"},
  };
  for (auto& s : sensors) {
    String topic = base + s[0] + "/config";
    String payload = "{\"name\":\"" + String(s[1]) + "\",\"state_topic\":\"" + topicFor(s[0]) + "/state\"," + dev + "}";
    mqtt.publish(topic.c_str(), payload.c_str(), true);
  }

  String lightTopic = base + "light/strip/config";
  String lightPayload = "{\"name\":\"LED Strip\",\"state_topic\":\"" + topicFor("light/state") + "\",\"command_topic\":\"" + topicFor("light/set") + "\",\"brightness\":true,\"brightness_scale\":255," + dev + "}";
  mqtt.publish(lightTopic.c_str(), lightPayload.c_str(), true);

  discoverySent = true;
}

void mqttSetup() {
  mqtt.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
  mqtt.setBufferSize(512);

  xTaskCreatePinnedToCore(mqttConnectTask, "mqtt", 4096, nullptr, 1, nullptr, tskNO_AFFINITY);
}

void mqttLoop() {
  if (!mqtt.connected()) return;
  mqtt.loop();
  if (!discoverySent) {
    sendDiscovery();
  }
}

void mqttPublishAll(bool presence, int distanceCm, bool lightOn, uint8_t brightness,
                    int potValue) {
  if (!mqtt.connected()) return;

  mqtt.publish(topicFor("binary_sensor/presence/state").c_str(), presence ? "ON" : "OFF", true);
  mqtt.publish(topicFor("sensor/radar_distance/state").c_str(), String(distanceCm).c_str(), true);

  JsonDocument lightDoc;
  lightDoc["state"] = lightOn ? "ON" : "OFF";
  lightDoc["brightness"] = brightness;
  char lightBuf[64];
  serializeJson(lightDoc, lightBuf, sizeof(lightBuf));
  mqtt.publish(topicFor("light/state").c_str(), lightBuf, true);

  mqtt.publish(topicFor("sensor/brightness_pot/state").c_str(), String(potValue).c_str(), true);
}
