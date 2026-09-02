#include "web_server.h"
#include "config.h"
#include "wifi_manager.h"
#include "gas_sensor.h"
#include "temperature_sensor.h"
#include "inputs.h"
#include "outputs.h"
#include "radar.h"
#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);

static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LED on Presence</title>
  <style>
    body { font-family: system-ui; max-width: 400px; margin: 40px auto; padding: 0 20px; }
    h1 { font-size: 1.4em; }
    .row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }
    .label { color: #666; }
    .on { color: #2d2; font-weight: bold; }
    .off { color: #d22; }
    button { margin-top: 16px; padding: 10px 20px; font-size: 1em; cursor: pointer; }
  </style>
</head>
<body>
  <h1>LED on Presence</h1>
  <div class="row"><span class="label">Mode</span><span id="mode">-</span></div>
  <div class="row"><span class="label">Presence</span><span id="pres">-</span></div>
  <div class="row"><span class="label">Distance</span><span id="dist">-</span></div>
  <div class="row"><span class="label">Light</span><span id="light">-</span></div>
  <div class="row"><span class="label">Brightness</span><span id="bright">-</span></div>
  <div class="row"><span class="label">Pot</span><span id="pot">-</span></div>
  <div class="row"><span class="label">Gas Level</span><span id="gas">-</span></div>
  <div class="row"><span class="label">Gas Alarm</span><span id="alarm">-</span></div>
  <div class="row"><span class="label">Gas Threshold</span><span id="thresh">-</span></div>
  <div class="row"><span class="label">Temperature</span><span id="temp">-</span></div>
  <div class="row"><span class="label">Humidity</span><span id="hum">-</span></div>
  <script>
    function update() {
      fetch('/api/status').then(r => r.json()).then(d => {
        document.getElementById('mode').textContent = d.mode;
        document.getElementById('mode').className = d.mode === 'MANUAL' ? 'on' : '';
        document.getElementById('pres').textContent = d.presence ? 'YES' : 'NO';
        document.getElementById('pres').className = d.presence ? 'on' : 'off';
        document.getElementById('dist').textContent = d.presence ? d.distance + ' cm' : '-';
        document.getElementById('light').textContent = d.lightOn ? 'ON' : 'OFF';
        document.getElementById('light').className = d.lightOn ? 'on' : 'off';
        document.getElementById('bright').textContent = d.brightness + ' / 255';
        document.getElementById('pot').textContent = d.potValue;
        document.getElementById('gas').textContent = d.gasLevel;
        document.getElementById('alarm').textContent = d.gasAlarm ? 'ALARM' : 'OK';
        document.getElementById('alarm').className = d.gasAlarm ? 'off' : 'on';
        document.getElementById('thresh').textContent = d.gasThreshold;
        document.getElementById('temp').textContent = d.temperature !== null ? d.temperature + ' °C' : '-';
        document.getElementById('hum').textContent = d.humidity !== null ? d.humidity + ' %' : '-';
      });
    }
    update();
    setInterval(update, 1000);
  </script>
</body>
</html>
)rawliteral";

void webServerSetup() {
  if (!wifiIsConnected()) return;

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send(200, "text/html", PAGE_HTML);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    JsonDocument doc;
    doc["mode"] = getMode() == MODE_MOTION ? "MOTION" : "MANUAL";
    doc["presence"] = radarPresenceDetected();
    doc["distance"] = radarDetectedDistance();
    doc["lightOn"] = isLightOn();
    doc["brightness"] = getCurrentBrightness();
    doc["potValue"] = getPotValue();
    doc["gasLevel"] = gasReadAnalog();
    doc["gasAlarm"] = gasIsAlarm();
    doc["gasThreshold"] = gasGetThreshold();
    if (temperatureIsValid()) {
      doc["temperature"] = temperatureGetCelsius();
      doc["humidity"] = temperatureGetHumidity();
    } else {
      doc["temperature"] = nullptr;
      doc["humidity"] = nullptr;
    }
    char buf[256];
    serializeJson(doc, buf, sizeof(buf));
    req->send(200, "application/json", buf);
  });

  server.begin();
  Serial.println(F("Web: server started on port 80"));
}

void webServerLoop() {
}
